#include "AiChatClient.h"
#include <algorithm>
#include <sstream>
#include <regex>
#include <random>
#include <iomanip>

namespace aichat {

// =============================================================================
// AiChatClient Implementation
// =============================================================================

AiChatClient::AiChatClient()
    : AiChatClient(Config{})
{
}

AiChatClient::AiChatClient(const Config& config)
    : mResponseManager(std::make_unique<ResponseManager>(config.mMaxConcurrentCalls))
    , mState(std::make_unique<ConversationState>())
    , mConfig(config)
    , mSentencesSinceLastBackendCall(0)
{
    createTriggerForLanguage(config.mLanguage);
}

AiChatClient::~AiChatClient() = default;

void AiChatClient::streamSentence(const std::string& sentence) {
    if (sentence.empty() || !mState->canAcceptSentences()) {
        return;
    }

    // Auto-detect language and switch trigger if needed
    if (mConfig.mLanguage == Language::Auto) {
        Language detectedLang = detectLanguage(sentence);

        // Switch trigger if detected language doesn't match current trigger
        if ((detectedLang == Language::Korean &&
             dynamic_cast<KoreanBackendTrigger*>(mTrigger.get()) == nullptr) ||
            (detectedLang == Language::English &&
             dynamic_cast<EnglishBackendTrigger*>(mTrigger.get()) == nullptr)) {
            createTriggerForLanguage(detectedLang);
        }
    }

    // Add sentence to conversation
    addSentenceToConversation(sentence);
    mSentencesSinceLastBackendCall++;
    mTrigger->updateLastActivity();

    // Update state if this is the first sentence
    if (mState->getState() == ConversationState::State::Idle) {
        mState->markConversationStart();
        mState->setState(ConversationState::State::Accumulating);
    }

    // Determine if we should trigger a backend call and what to send
    bool shouldCallBackend = false;
    std::string conversationToSend;

    // Priority 1: Smart triggers (send full conversation)
    if (mConfig.mEnableSmartTriggers) {
        const std::string& fullConversation = getFullConversation();
        // Check if the FULL conversation (not just current sentence) meets trigger criteria
        if (mTrigger->shouldTrigger(fullConversation) || mTrigger->shouldTriggerOnTimeout()) {
            // Only trigger if we don't already have a pending request for this conversation
            if (!mResponseManager->hasPendingConversation(fullConversation)) {
                conversationToSend = fullConversation;
                shouldCallBackend = true;
            }
        }
    }
    // Priority 2: Chunking (only if smart triggers didn't fire)
    else if (mConfig.mEnableChunking && mSentencesSinceLastBackendCall >= mConfig.mChunkSize) {
        if (mTrigger->shouldTrigger(sentence)) {
            const std::string& fullConversation = getFullConversation();
            // Only trigger if we don't already have a pending request for this conversation
            if (!mResponseManager->hasPendingConversation(fullConversation)) {
                conversationToSend = fullConversation;
                shouldCallBackend = true;
            }
        }
    }

    // Make the backend call if needed
    if (shouldCallBackend) {
        std::string requestId = generateRequestId();
        handleTriggerEvent(conversationToSend, requestId);
        // Reset counter since we just made a backend call
        mSentencesSinceLastBackendCall = 0;
    }
}

void AiChatClient::endConversation() {
    if (mState->getState() == ConversationState::State::Idle) {
        return;
    }

    mState->markConversationEnd();
    mState->setState(ConversationState::State::WaitingForEnd);

    // Check if we already have a cached response
    if (mResponseManager->hasCachedResponse()) {
        sendFinalResponse();
        return;
    }

    // Send final conversation if no cached response available
    if (!isConversationEmpty()) {
        const std::string& conversation = getFullConversation();
        // Only send if we don't already have a pending request for this conversation
        if (!mResponseManager->hasPendingConversation(conversation)) {
            std::string requestId = generateRequestId();
            handleTriggerEvent(conversation, requestId);
            // Reset counter since we just made a backend call
            mSentencesSinceLastBackendCall = 0;
        }
    }

    // Wait for response or timeout
    sendFinalResponse();
}

void AiChatClient::setBackendCallback(BackendCallback callback) {
    mBackendCallback = std::move(callback);
}

void AiChatClient::setResponseCallback(ResponseCallback callback) {
    mResponseCallback = std::move(callback);
}

void AiChatClient::setErrorCallback(ErrorCallback callback) {
    mErrorCallback = std::move(callback);
}

bool AiChatClient::isProcessing() const {
    return mState->isProcessing() || mResponseManager->getPendingCount() > 0;
}

void AiChatClient::reset() {
    clearConversation();
    mTrigger->reset();
    mResponseManager->clear();
    mState->reset();
    mSentencesSinceLastBackendCall = 0;
}

void AiChatClient::updateConfig(const Config& config) {
    mConfig = config;
    // Recreate components with new config
    createTriggerForLanguage(config.mLanguage);
    mResponseManager = std::make_unique<ResponseManager>(config.mMaxConcurrentCalls);
}

void AiChatClient::handleTriggerEvent(const std::string& conversation, const std::string& triggerId) {
    if (!mBackendCallback) {
        handleError("Backend callback not set");
        return;
    }

    // Add to pending requests
    mResponseManager->addPendingRequest(triggerId, conversation);
    mState->markProcessingStart();

    // Create response handler
    auto responseHandler = [this, triggerId](const std::string& response) {
        handleBackendResponse(response, triggerId);
    };

    // Call backend
    try {
        mBackendCallback(conversation, responseHandler);
    } catch (const std::exception& e) {
        handleError("Backend call failed: " + std::string(e.what()));
    }
}

void AiChatClient::handleBackendResponse(const std::string& response, const std::string& requestId) {
    mResponseManager->handleResponse(requestId, response);

    // If we're waiting for end conversation, check if we can send final response
    if (mState->getState() == ConversationState::State::WaitingForEnd) {
        sendFinalResponse();
    }
}

void AiChatClient::handleError(const std::string& error) {
    if (mErrorCallback) {
        mErrorCallback(error);
    }
}

void AiChatClient::sendFinalResponse() {
    if (!mResponseCallback) {
        return;
    }

    std::string finalResponse;

    if (mResponseManager->hasCachedResponse()) {
        finalResponse = mResponseManager->getMergedResponse();
    } else {
        finalResponse = "No response available";
    }

    mResponseCallback(finalResponse);
    mState->markProcessingComplete();
    mState->setState(ConversationState::State::Completed);
}

bool AiChatClient::shouldTriggerBackendCall(const std::string& sentence) const {
    return mTrigger->shouldTrigger(sentence) || mTrigger->shouldTriggerOnTimeout();
}

std::string AiChatClient::generateRequestId() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);

    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    return "req_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));
}

void AiChatClient::createTriggerForLanguage(Language language) {
    switch (language) {
        case Language::English:
            mTrigger = std::make_unique<EnglishBackendTrigger>(mConfig);
            break;
        case Language::Korean:
            mTrigger = std::make_unique<KoreanBackendTrigger>(mConfig);
            break;
        case Language::Auto:
            // Default to English, will switch dynamically based on content
            mTrigger = std::make_unique<EnglishBackendTrigger>(mConfig);
            break;
    }
}

AiChatClient::Language AiChatClient::detectLanguage(const std::string& sentence) const {
    // Simple heuristic: check for Korean characters (Hangul)
    for (const char* ptr = sentence.c_str(); *ptr != '\0';) {
        unsigned char c = static_cast<unsigned char>(*ptr);

        // Check for UTF-8 Korean characters (Hangul range: 0xAC00-0xD7AF)
        if (c >= 0xE0 && c <= 0xEF) {  // 3-byte UTF-8 sequence
            if (ptr[1] && ptr[2]) {
                uint32_t codepoint = ((c & 0x0F) << 12) |
                                   ((static_cast<unsigned char>(ptr[1]) & 0x3F) << 6) |
                                   (static_cast<unsigned char>(ptr[2]) & 0x3F);

                // Korean Hangul syllables range
                if (codepoint >= 0xAC00 && codepoint <= 0xD7AF) {
                    return Language::Korean;
                }
                ptr += 3;
            } else {
                ptr++;
            }
        } else if (c >= 0x80) {
            // Skip other multi-byte sequences
            if (c >= 0xF0) ptr += 4;
            else if (c >= 0xE0) ptr += 3;
            else if (c >= 0xC0) ptr += 2;
            else ptr++;
        } else {
            ptr++;
        }
    }

    return Language::English;  // Default to English if no Korean detected
}

// =============================================================================
// Conversation Management Methods
// =============================================================================

void AiChatClient::addSentenceToConversation(const std::string& sentence) {
    if (!sentence.empty()) {
        if (!mConversation.empty()) {
            mConversation += " ";
        }
        mConversation += sentence;
    }
}

const std::string& AiChatClient::getFullConversation() const {
    return mConversation;
}

void AiChatClient::clearConversation() {
    mConversation.clear();
}

bool AiChatClient::isConversationEmpty() const {
    return mConversation.empty();
}

// =============================================================================
// BackendTriggerBase Implementation
// =============================================================================

AiChatClient::BackendTriggerBase::BackendTriggerBase(const Config& config)
    : mConfig(config)
    , mLastActivity(std::chrono::steady_clock::now())
{
}

bool AiChatClient::BackendTriggerBase::shouldTriggerOnTimeout() const {
    return isTimeoutReached();
}

void AiChatClient::BackendTriggerBase::updateLastActivity() {
    mLastActivity = std::chrono::steady_clock::now();
}

void AiChatClient::BackendTriggerBase::reset() {
    mLastActivity = std::chrono::steady_clock::now();
}

bool AiChatClient::BackendTriggerBase::isTimeoutReached() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - mLastActivity);
    return elapsed.count() >= static_cast<long>(mConfig.mTriggerTimeoutMs);
}

// =============================================================================
// EnglishBackendTrigger Implementation
// =============================================================================

AiChatClient::EnglishBackendTrigger::EnglishBackendTrigger(const Config& config)
    : BackendTriggerBase(config)
{
}

bool AiChatClient::EnglishBackendTrigger::shouldTrigger(const std::string& sentence) const {
    if (!mConfig.mEnableSmartTriggers) {
        return false;
    }

    return hasPunctuation(sentence) || hasQuestionPattern(sentence);
}

bool AiChatClient::EnglishBackendTrigger::hasPunctuation(const std::string& sentence) const {
    if (sentence.empty()) {
        return false;
    }

    char lastChar = sentence.back();
    return lastChar == '.' || lastChar == '!' || lastChar == '?';
}

bool AiChatClient::EnglishBackendTrigger::hasQuestionPattern(const std::string& sentence) const {
    // Convert to lowercase for pattern matching
    std::string lowerSentence = sentence;
    std::transform(lowerSentence.begin(), lowerSentence.end(), lowerSentence.begin(), ::tolower);

    static const std::vector<std::string> questionWords = {
        "what", "how", "when", "where", "why", "who", "which", "can", "could",
        "would", "should", "will", "is", "are", "do", "does", "did"
    };

    // Look for question words and require punctuation to indicate completeness
    for (const auto& word : questionWords) {
        if (lowerSentence.find(word) != std::string::npos) {
            // Only trigger if sentence also has punctuation (indicating completeness)
            return hasPunctuation(sentence);
        }
    }

    return false;
}

// =============================================================================
// KoreanBackendTrigger Implementation
// =============================================================================

AiChatClient::KoreanBackendTrigger::KoreanBackendTrigger(const Config& config)
    : BackendTriggerBase(config)
{
}

bool AiChatClient::KoreanBackendTrigger::shouldTrigger(const std::string& sentence) const {
    if (!mConfig.mEnableSmartTriggers) {
        return false;
    }

    return hasPunctuation(sentence) || hasQuestionPattern(sentence);
}

bool AiChatClient::KoreanBackendTrigger::hasPunctuation(const std::string& sentence) const {
    if (sentence.empty()) {
        return false;
    }

    // Korean punctuation (UTF-8 encoded)
    static const std::vector<std::string> koreanPunctuation = {
        "。", "！", "？", ".", "~", "요.", "다.", "죠.", "네.", "야.", "지?"
    };

    // Also check for standard punctuation
    char lastChar = sentence.back();
    if (lastChar == '.' || lastChar == '!' || lastChar == '?') {
        return true;
    }

    for (const auto& punct : koreanPunctuation) {
        if (sentence.length() >= punct.length()) {
            if (sentence.substr(sentence.length() - punct.length()) == punct) {
                return true;
            }
        }
    }

    return false;
}

bool AiChatClient::KoreanBackendTrigger::hasQuestionPattern(const std::string& sentence) const {
    // Korean question words (in UTF-8)
    static const std::vector<std::string> koreanQuestionWords = {
        "뭐", "무엇", "언제", "어디", "왜", "누구", "어떻게", "어느",
        "몇", "얼마", "어떤", "할까", "인가", "습니까", "ㅂ니까",
        "는가", "나요", "까요", "을까", "를까"
    };

    // Korean question endings/particles
    static const std::vector<std::string> koreanQuestionEndings = {
        "까", "가", "니", "요", "나", "냐", "어", "야", "지", "ㅏ", "ㅓ", "ㅗ", "ㅜ"
    };

    // Check Korean question words - but require punctuation for completeness
    for (const auto& word : koreanQuestionWords) {
        if (sentence.find(word) != std::string::npos) {
            // Only trigger if sentence also has punctuation (indicating completeness)
            return hasPunctuation(sentence);
        }
    }

    // Check Korean question endings (at the end of sentence)
    for (const auto& ending : koreanQuestionEndings) {
        if (sentence.length() >= ending.length()) {
            if (sentence.substr(sentence.length() - ending.length()) == ending) {
                // Question endings themselves indicate completeness
                return true;
            }
        }
    }

    return false;
}

// =============================================================================
// ResponseManager Implementation
// =============================================================================

AiChatClient::ResponseManager::ResponseManager(uint32_t maxConcurrentCalls)
    : mMaxConcurrentCalls(maxConcurrentCalls)
{
}

void AiChatClient::ResponseManager::addPendingRequest(const std::string& requestId, const std::string& conversation) {
    // Clean up old requests first
    invalidateOldRequests();

    // Remove oldest request if we exceed limit
    if (mRequests.size() >= mMaxConcurrentCalls) {
        auto oldestIt = std::min_element(mRequests.begin(), mRequests.end(),
            [](const auto& a, const auto& b) {
                return a.second.timestamp < b.second.timestamp;
            });
        if (oldestIt != mRequests.end()) {
            mRequests.erase(oldestIt);
        }
    }

    RequestInfo info;
    info.conversation = conversation;
    info.timestamp = std::chrono::steady_clock::now();
    info.isComplete = false;

    mRequests[requestId] = info;
}

void AiChatClient::ResponseManager::handleResponse(const std::string& requestId, const std::string& response) {
    auto it = mRequests.find(requestId);
    if (it != mRequests.end()) {
        it->second.response = response;
        it->second.isComplete = true;
    }
}

void AiChatClient::ResponseManager::invalidateOldRequests() {
    for (auto it = mRequests.begin(); it != mRequests.end();) {
        if (isRequestExpired(it->second)) {
            it = mRequests.erase(it);
        } else {
            ++it;
        }
    }
}

bool AiChatClient::ResponseManager::hasCachedResponse() const {
    for (const auto& pair : mRequests) {
        if (pair.second.isComplete) {
            return true;
        }
    }
    return false;
}



std::string AiChatClient::ResponseManager::getMergedResponse() const {
    std::vector<std::string> responses;

    for (const auto& pair : mRequests) {
        if (pair.second.isComplete && !pair.second.response.empty()) {
            responses.push_back(pair.second.response);
        }
    }

    if (responses.empty()) {
        return "";
    }

    if (responses.size() == 1) {
        return responses[0];
    }

    // For multiple responses, return the longest one (most complete)
    auto maxIt = std::max_element(responses.begin(), responses.end(),
        [](const std::string& a, const std::string& b) {
            return a.length() < b.length();
        });

    return *maxIt;
}

void AiChatClient::ResponseManager::clear() {
    mRequests.clear();
}

size_t AiChatClient::ResponseManager::getPendingCount() const {
    size_t count = 0;
    for (const auto& pair : mRequests) {
        if (!pair.second.isComplete) {
            count++;
        }
    }
    return count;
}

bool AiChatClient::ResponseManager::hasPendingConversation(const std::string& conversation) const {
    for (const auto& pair : mRequests) {
        if (!pair.second.isComplete && pair.second.conversation == conversation) {
            return true;
        }
    }
    return false;
}



bool AiChatClient::ResponseManager::isRequestExpired(const RequestInfo& request) const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - request.timestamp);
    return elapsed.count() > REQUEST_TIMEOUT_MS;
}

// =============================================================================
// ConversationState Implementation
// =============================================================================

AiChatClient::ConversationState::ConversationState()
    : mCurrentState(State::Idle)
{
}

void AiChatClient::ConversationState::setState(State state) {
    mCurrentState = state;
}

AiChatClient::ConversationState::State AiChatClient::ConversationState::getState() const {
    return mCurrentState;
}

void AiChatClient::ConversationState::markConversationStart() {
    setState(State::Accumulating);
}

void AiChatClient::ConversationState::markConversationEnd() {
    // Nothing to do - just marking the end for state management
}

void AiChatClient::ConversationState::markProcessingStart() {
    if (mCurrentState == State::Accumulating) {
        setState(State::Processing);
    }
}

void AiChatClient::ConversationState::markProcessingComplete() {
    setState(State::Completed);
}

bool AiChatClient::ConversationState::isProcessing() const {
    return mCurrentState == State::Processing || mCurrentState == State::WaitingForEnd;
}

bool AiChatClient::ConversationState::canAcceptSentences() const {
    return mCurrentState == State::Idle ||
           mCurrentState == State::Accumulating ||
           mCurrentState == State::Processing;
}



void AiChatClient::ConversationState::reset() {
    mCurrentState = State::Idle;
}

} // namespace aichat
