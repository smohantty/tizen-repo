#include "AiChatClient.h"
#include <algorithm>
#include <sstream>
#include <regex>
#include <random>
#include <iomanip>
#include <unordered_map>
#include <thread>

namespace aichat {

// Type aliases for cleaner code
using Config = AiChatClient::Config;
using Language = AiChatClient::Language;
using BackendCallback = AiChatClient::BackendCallback;
using ResponseCallback = AiChatClient::ResponseCallback;
using ErrorCallback = AiChatClient::ErrorCallback;

// =============================================================================
// Forward Declarations for Implementation Classes
// =============================================================================

class BackendTriggerBase {
public:
    explicit BackendTriggerBase(const Config& config)
        : mConfig(config)
        , mLastActivity(std::chrono::steady_clock::now())
    {
    }

    virtual ~BackendTriggerBase() = default;

    virtual bool shouldTrigger(const std::string& sentence) const = 0;

    virtual bool shouldTriggerOnTimeout() const {
        return isTimeoutReached();
    }

    virtual void updateLastActivity() {
        mLastActivity = std::chrono::steady_clock::now();
    }

    virtual void reset() {
        mLastActivity = std::chrono::steady_clock::now();
    }

protected:
    Config mConfig;
    std::chrono::steady_clock::time_point mLastActivity;

    virtual bool hasPunctuation(const std::string& sentence) const = 0;
    virtual bool hasQuestionPattern(const std::string& sentence) const = 0;

    bool isTimeoutReached() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - mLastActivity);
        return elapsed.count() >= static_cast<long>(mConfig.mTriggerTimeoutMs);
    }
};

class EnglishBackendTrigger : public BackendTriggerBase {
public:
    explicit EnglishBackendTrigger(const Config& config)
        : BackendTriggerBase(config)
    {
    }

    bool shouldTrigger(const std::string& sentence) const override {
        if (sentence.empty()) {
            return false;
        }
        return hasPunctuation(sentence) || hasQuestionPattern(sentence);
    }

protected:
    bool hasPunctuation(const std::string& sentence) const override {
        return sentence.find_last_of(".!?") != std::string::npos;
    }

    bool hasQuestionPattern(const std::string& sentence) const override {
        // Convert to lowercase for case-insensitive matching
        std::string lowerSentence = sentence;
        std::transform(lowerSentence.begin(), lowerSentence.end(), lowerSentence.begin(), ::tolower);

        // English question patterns
        std::vector<std::string> questionWords = {
            "what", "how", "when", "where", "why", "who", "which", "whose",
            "can you", "could you", "would you", "will you", "should",
            "do you", "did you", "are you", "is it", "have you"
        };

        for (const auto& word : questionWords) {
            if (lowerSentence.find(word) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
};

class KoreanBackendTrigger : public BackendTriggerBase {
public:
    explicit KoreanBackendTrigger(const Config& config)
        : BackendTriggerBase(config)
    {
    }

    bool shouldTrigger(const std::string& sentence) const override {
        if (sentence.empty()) {
            return false;
        }
        return hasPunctuation(sentence) || hasQuestionPattern(sentence);
    }

protected:
    bool hasPunctuation(const std::string& sentence) const override {
        // Korean punctuation marks
        std::vector<std::string> koreanPunct = {".", "!", "?", "。", "！", "？"};

        for (const auto& punct : koreanPunct) {
            if (sentence.find(punct) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    bool hasQuestionPattern(const std::string& sentence) const override {
        // Korean question patterns
        std::vector<std::string> questionPatterns = {
            "뭐", "무엇", "어떻", "어디", "언제", "왜", "누구", "몇",
            "까요", "습니까", "나요", "죠", "지요",
            "할까", "어떨까", "괜찮", "어때"
        };

        for (const auto& pattern : questionPatterns) {
            if (sentence.find(pattern) != std::string::npos) {
                return true;
            }
        }

        // Check for question ending patterns
        if (sentence.find("요?") != std::string::npos ||
            sentence.find("까?") != std::string::npos ||
            sentence.find("나?") != std::string::npos) {
            return true;
        }

        return false;
    }
};

class ResponseManager {
public:
    explicit ResponseManager(uint32_t maxConcurrentCalls)
    {
        // Note: maxConcurrentCalls parameter kept for API compatibility
        // but not currently used in implementation
        (void)maxConcurrentCalls;  // Suppress unused parameter warning
    }

    void addPendingRequest(const std::string& requestId, const std::string& conversation) {
        invalidateOldRequests();
        RequestInfo request;
        request.conversation = conversation;
        request.timestamp = std::chrono::steady_clock::now();
        request.isComplete = false;
        mRequests[requestId] = request;
    }

    void handleResponse(const std::string& requestId, const std::string& response) {
        auto it = mRequests.find(requestId);
        if (it != mRequests.end()) {
            it->second.response = response;
            it->second.isComplete = true;
        }
    }

    void invalidateOldRequests() {
        auto it = mRequests.begin();
        while (it != mRequests.end()) {
            if (isRequestExpired(it->second)) {
                it = mRequests.erase(it);
            } else {
                ++it;
            }
        }
    }

    bool hasCachedResponse() const {
        for (const auto& pair : mRequests) {
            if (pair.second.isComplete && !pair.second.response.empty()) {
                return true;
            }
        }
        return false;
    }

    std::string getMergedResponse() const {
        // Return the response for the most complete conversation (longest text)
        // This represents the response to the full conversation context
        std::string bestResponse;
        size_t longestConversationLength = 0;

        for (const auto& pair : mRequests) {
            if (pair.second.isComplete && !pair.second.response.empty()) {
                // Find the response corresponding to the longest/most complete conversation
                if (pair.second.conversation.length() > longestConversationLength) {
                    longestConversationLength = pair.second.conversation.length();
                    bestResponse = pair.second.response;
                }
            }
        }
        return bestResponse;
    }

    void clear() {
        mRequests.clear();
    }

    size_t getPendingCount() const {
        size_t count = 0;
        for (const auto& pair : mRequests) {
            if (!pair.second.isComplete) {
                count++;
            }
        }
        return count;
    }

    bool hasPendingConversation(const std::string& conversation) const {
        for (const auto& pair : mRequests) {
            if (pair.second.conversation == conversation && !pair.second.isComplete) {
                return true;
            }
        }
        return false;
    }

private:
    struct RequestInfo {
        std::string conversation;
        std::string response;
        std::chrono::steady_clock::time_point timestamp;
        bool isComplete;
    };

    std::unordered_map<std::string, RequestInfo> mRequests;

    bool isRequestExpired(const RequestInfo& request) const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - request.timestamp);
        return elapsed.count() >= REQUEST_TIMEOUT_MS;
    }

    static const uint32_t REQUEST_TIMEOUT_MS = 10000; // 10 seconds
};

class ConversationState {
public:
    enum class State {
        Idle,
        Accumulating,
        Processing,
        WaitingForEnd,
        Completed
    };

    ConversationState()
        : mCurrentState(State::Idle)
    {
    }

    void setState(State state) {
        mCurrentState = state;
    }

    State getState() const {
        return mCurrentState;
    }

    void markConversationStart() {
        setState(State::Accumulating);
    }

    void markConversationEnd() {
        // Nothing to do - just marking the end for state management
    }

    void markProcessingStart() {
        if (mCurrentState == State::Accumulating) {
            setState(State::Processing);
        }
    }

    void markProcessingComplete() {
        setState(State::Completed);
    }

    bool isProcessing() const {
        return mCurrentState == State::Processing || mCurrentState == State::WaitingForEnd;
    }

    bool canAcceptSentences() const {
        return mCurrentState == State::Idle ||
               mCurrentState == State::Accumulating ||
               mCurrentState == State::Processing;
    }

    void reset() {
        mCurrentState = State::Idle;
    }

private:
    State mCurrentState;
};

// =============================================================================
// AiChatClient::Impl - Implementation Class (PIMPL)
// =============================================================================

class AiChatClient::Impl {
public:
    explicit Impl(const Config& config)
        : mResponseManager(std::make_unique<ResponseManager>(config.mMaxConcurrentCalls))
        , mState(std::make_unique<ConversationState>())
        , mConfig(config)
        , mSentencesSinceLastBackendCall(0)
    {
        createTriggerForLanguage(config.mLanguage);
    }

    ~Impl() = default;

    // Core components
    std::string mConversation;
    std::unique_ptr<BackendTriggerBase> mTrigger;
    std::unique_ptr<ResponseManager> mResponseManager;
    std::unique_ptr<ConversationState> mState;

    // Configuration and callbacks
    Config mConfig;
    BackendCallback mBackendCallback;
    ResponseCallback mResponseCallback;
    ErrorCallback mErrorCallback;

    // Chunking state tracking
    size_t mSentencesSinceLastBackendCall;

    // Internal methods
    void handleTriggerEvent(const std::string& conversation, const std::string& triggerId) {
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

        // Create error handler
        auto errorHandler = [this](const std::string& error) {
            handleError(error);
        };

        // Call backend API
        try {
            mBackendCallback(conversation, responseHandler);
        } catch (const std::exception& e) {
            errorHandler("Backend callback failed: " + std::string(e.what()));
        }
    }

    void handleBackendResponse(const std::string& response, const std::string& requestId) {
        // Cache the response for later use when endConversation() is called
        mResponseManager->handleResponse(requestId, response);

        // Do NOT send response to user immediately - this defeats the latency optimization
        // The response will be sent only when endConversation() is called via sendFinalResponse()

        // Note: We don't mark processing as complete yet since user hasn't called endConversation()
        // Processing is complete only when the final response is delivered to the user
    }

    void handleError(const std::string& error) {
        if (mErrorCallback) {
            mErrorCallback(error);
        }
    }

    void sendFinalResponse() {
        if (mResponseManager->hasCachedResponse()) {
            std::string response = mResponseManager->getMergedResponse();
            if (mResponseCallback && !response.empty()) {
                mResponseCallback(response);
            }
        } else {
            // No response available
            if (mResponseCallback) {
                mResponseCallback("No response available");
            }
        }

        // Mark conversation as completed
        mState->markProcessingComplete();

        // Clean up old responses
        mResponseManager->clear();
    }

    bool shouldTriggerBackendCall(const std::string& sentence) const {
        return mTrigger->shouldTrigger(sentence) || mTrigger->shouldTriggerOnTimeout();
    }

    std::string generateRequestId() const {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        ss << std::hex;
        for (int i = 0; i < 8; i++) {
            ss << dis(gen);
        }
        return ss.str();
    }

    void createTriggerForLanguage(Language language) {
        switch (language) {
            case Language::English:
                mTrigger = std::make_unique<EnglishBackendTrigger>(mConfig);
                break;
            case Language::Korean:
                mTrigger = std::make_unique<KoreanBackendTrigger>(mConfig);
                break;
            case Language::Auto:
            default:
                // Start with English trigger as default
                mTrigger = std::make_unique<EnglishBackendTrigger>(mConfig);
                break;
        }
    }

    Language detectLanguage(const std::string& sentence) const {
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

    // Conversation management helpers
    void addSentenceToConversation(const std::string& sentence) {
        if (!sentence.empty()) {
            if (!mConversation.empty()) {
                mConversation += " ";
            }
            mConversation += sentence;
        }
    }

    const std::string& getFullConversation() const {
        return mConversation;
    }

    void clearConversation() {
        mConversation.clear();
    }

    bool isConversationEmpty() const {
        return mConversation.empty();
    }
};

// =============================================================================
// AiChatClient Implementation
// =============================================================================

AiChatClient::AiChatClient()
    : AiChatClient(Config{})
{
}

AiChatClient::AiChatClient(const Config& config)
    : mPImpl(std::make_unique<Impl>(config))
{
}

AiChatClient::~AiChatClient() = default;

void AiChatClient::streamSentence(const std::string& sentence) {
    if (sentence.empty() || !mPImpl->mState->canAcceptSentences()) {
        return;
    }

    // Auto-detect language and switch trigger if needed
    if (mPImpl->mConfig.mLanguage == Language::Auto) {
        Language detectedLang = mPImpl->detectLanguage(sentence);

        // Switch trigger if detected language doesn't match current trigger
        if ((detectedLang == Language::Korean &&
             dynamic_cast<KoreanBackendTrigger*>(mPImpl->mTrigger.get()) == nullptr) ||
            (detectedLang == Language::English &&
             dynamic_cast<EnglishBackendTrigger*>(mPImpl->mTrigger.get()) == nullptr)) {
            mPImpl->createTriggerForLanguage(detectedLang);
        }
    }

    // Add sentence to conversation
    mPImpl->addSentenceToConversation(sentence);
    mPImpl->mSentencesSinceLastBackendCall++;
    mPImpl->mTrigger->updateLastActivity();

    // Update state if this is the first sentence
    if (mPImpl->mState->getState() == ConversationState::State::Idle) {
        mPImpl->mState->markConversationStart();
        mPImpl->mState->setState(ConversationState::State::Accumulating);
    }

    // Determine if we should trigger a backend call and what to send
    bool shouldCallBackend = false;
    std::string conversationToSend;

    // Priority 1: Smart triggers (send full conversation)
    if (mPImpl->mConfig.mEnableSmartTriggers) {
        const std::string& fullConversation = mPImpl->getFullConversation();
        // Check if the FULL conversation (not just current sentence) meets trigger criteria
        if (mPImpl->mTrigger->shouldTrigger(fullConversation) || mPImpl->mTrigger->shouldTriggerOnTimeout()) {
            // Only trigger if we don't already have a pending request for this conversation
            if (!mPImpl->mResponseManager->hasPendingConversation(fullConversation)) {
                conversationToSend = fullConversation;
                shouldCallBackend = true;
            }
        }
    }
    // Priority 2: Chunking (only if smart triggers didn't fire)
    else if (mPImpl->mConfig.mEnableChunking && mPImpl->mSentencesSinceLastBackendCall >= mPImpl->mConfig.mChunkSize) {
        if (mPImpl->mTrigger->shouldTrigger(sentence)) {
            const std::string& fullConversation = mPImpl->getFullConversation();
            // Only trigger if we don't already have a pending request for this conversation
            if (!mPImpl->mResponseManager->hasPendingConversation(fullConversation)) {
                conversationToSend = fullConversation;
                shouldCallBackend = true;
            }
        }
    }

    // Make the backend call if needed
    if (shouldCallBackend) {
        std::string requestId = mPImpl->generateRequestId();
        mPImpl->handleTriggerEvent(conversationToSend, requestId);
        // Reset counter since we just made a backend call
        mPImpl->mSentencesSinceLastBackendCall = 0;
    }
}

void AiChatClient::endConversation() {
    if (mPImpl->mState->getState() == ConversationState::State::Idle) {
        return;
    }

    mPImpl->mState->markConversationEnd();
    mPImpl->mState->setState(ConversationState::State::WaitingForEnd);

    // Wait for pending responses before checking cache (up to 2 seconds)
    const int maxWaitMs = 2000;
    const int checkIntervalMs = 50;
    int waitedMs = 0;

    while (waitedMs < maxWaitMs &&
           mPImpl->mResponseManager->getPendingCount() > 0 &&
           !mPImpl->mResponseManager->hasCachedResponse()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
        waitedMs += checkIntervalMs;
    }

    // Check if we now have a cached response after waiting
    if (mPImpl->mResponseManager->hasCachedResponse()) {
        mPImpl->sendFinalResponse();
        return;
    }

    // Send final conversation if no cached response available
    if (!mPImpl->isConversationEmpty()) {
        const std::string& conversation = mPImpl->getFullConversation();
        // Only send if we don't already have a pending request for this conversation
        if (!mPImpl->mResponseManager->hasPendingConversation(conversation)) {
            std::string requestId = mPImpl->generateRequestId();
            mPImpl->handleTriggerEvent(conversation, requestId);
            // Reset counter since we just made a backend call
            mPImpl->mSentencesSinceLastBackendCall = 0;

            // Wait for this final response
            waitedMs = 0;
            while (waitedMs < maxWaitMs && !mPImpl->mResponseManager->hasCachedResponse()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMs));
                waitedMs += checkIntervalMs;
            }
        }
    }

    // Send final response (cached or "No response available")
    mPImpl->sendFinalResponse();
}

void AiChatClient::setBackendCallback(BackendCallback callback) {
    mPImpl->mBackendCallback = std::move(callback);
}

void AiChatClient::setResponseCallback(ResponseCallback callback) {
    mPImpl->mResponseCallback = std::move(callback);
}

void AiChatClient::setErrorCallback(ErrorCallback callback) {
    mPImpl->mErrorCallback = std::move(callback);
}

bool AiChatClient::isProcessing() const {
    return mPImpl->mState->isProcessing() || mPImpl->mResponseManager->getPendingCount() > 0;
}

void AiChatClient::reset() {
    mPImpl->clearConversation();
    mPImpl->mTrigger->reset();
    mPImpl->mResponseManager->clear();
    mPImpl->mState->reset();
    mPImpl->mSentencesSinceLastBackendCall = 0;
}

void AiChatClient::updateConfig(const Config& config) {
    mPImpl->mConfig = config;
    // Recreate components with new config
    mPImpl->createTriggerForLanguage(config.mLanguage);
    mPImpl->mResponseManager = std::make_unique<ResponseManager>(config.mMaxConcurrentCalls);
}

} // namespace aichat