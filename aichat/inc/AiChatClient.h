#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <unordered_map>

namespace aichat {

/**
 * @brief AI Chat Client - Pure latency optimization layer
 *
 * This class intelligently decides WHEN and HOW to call backend APIs
 * to minimize response time by using smart triggers and caching strategies.
 *
 * Features:
 * - Multi-language support with separate trigger engines:
 *   * English: Punctuation (., !, ?) and question words (what, how, when, etc.)
 *   * Korean: Korean punctuation and question patterns (뭐, 무엇, 까요, etc.)
 *   * Auto: Dynamic language detection and trigger switching
 * - Smart trigger detection for immediate backend calls
 * - Intelligent chunking for long conversations
 * - Response caching and merging
 * - Resource-efficient design (1KB memory limit)
 */
class AiChatClient {
public:
    // Language support enumeration
    enum class Language {
        English,
        Korean,
        Auto  // Auto-detect based on content
    };

    // Configuration for latency optimization
    struct Config {
        size_t mMaxBufferSize = 20;           // sentences (max 1KB total)
        uint32_t mTriggerTimeoutMs = 500;     // time-based trigger delay
        uint32_t mChunkSize = 3;              // sentences per chunk
        bool mEnableSmartTriggers = true;     // punctuation/pattern triggers
        bool mEnableChunking = true;          // send chunks before end
        uint32_t mMaxConcurrentCalls = 2;     // limit concurrent backend calls
        Language mLanguage = Language::Auto;  // language for trigger detection
    };

    // Backend API callback type
    using BackendCallback = std::function<void(const std::string& conversation,
                                              std::function<void(const std::string&)> responseHandler)>;

    // Response and error callbacks
    using ResponseCallback = std::function<void(const std::string&)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    // Constructor/Destructor
    AiChatClient();
    explicit AiChatClient(const Config& config);
    ~AiChatClient();

    // Delete copy constructor and assignment operator (following C++17 best practices)
    AiChatClient(const AiChatClient&) = delete;
    AiChatClient& operator=(const AiChatClient&) = delete;

    // PUBLIC API - Core functionality
    void streamSentence(const std::string& sentence);
    void endConversation();

    // Backend integration
    void setBackendCallback(BackendCallback callback);

    // Response handling
    void setResponseCallback(ResponseCallback callback);
    void setErrorCallback(ErrorCallback callback);

    // State management
    bool isProcessing() const;
    void reset();

    // Configuration
    void updateConfig(const Config& config);

private:
    // Forward declarations for Pimpl idiom
    class BackendTriggerBase;
    class EnglishBackendTrigger;
    class KoreanBackendTrigger;
    class ResponseManager;
    class ConversationState;

    // Pimpl idiom for clean interface
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
    void handleTriggerEvent(const std::string& conversation, const std::string& triggerId);
    void handleBackendResponse(const std::string& response, const std::string& requestId);
    void handleError(const std::string& error);
    void sendFinalResponse();
    bool shouldTriggerBackendCall(const std::string& sentence) const;
    std::string generateRequestId() const;
    void createTriggerForLanguage(Language language);
    Language detectLanguage(const std::string& sentence) const;

    // Conversation management helpers
    void addSentenceToConversation(const std::string& sentence);
    const std::string& getFullConversation() const;
    void clearConversation();
    bool isConversationEmpty() const;
};



/**
 * @brief Base class for backend trigger decision engines
 *
 * Abstract interface for language-specific trigger detection.
 */
class AiChatClient::BackendTriggerBase {
public:
    explicit BackendTriggerBase(const Config& config);
    virtual ~BackendTriggerBase() = default;

    virtual bool shouldTrigger(const std::string& sentence) const = 0;
    virtual bool shouldTriggerOnTimeout() const;
    virtual void updateLastActivity();
    virtual void reset();

protected:
    Config mConfig;
    std::chrono::steady_clock::time_point mLastActivity;

    virtual bool hasPunctuation(const std::string& sentence) const = 0;
    virtual bool hasQuestionPattern(const std::string& sentence) const = 0;
    bool isTimeoutReached() const;
};

/**
 * @brief English language backend trigger
 *
 * Implements English-specific trigger detection patterns.
 */
class AiChatClient::EnglishBackendTrigger : public BackendTriggerBase {
public:
    explicit EnglishBackendTrigger(const Config& config);

    bool shouldTrigger(const std::string& sentence) const override;

protected:
    bool hasPunctuation(const std::string& sentence) const override;
    bool hasQuestionPattern(const std::string& sentence) const override;
};

/**
 * @brief Korean language backend trigger
 *
 * Implements Korean-specific trigger detection patterns.
 */
class AiChatClient::KoreanBackendTrigger : public BackendTriggerBase {
public:
    explicit KoreanBackendTrigger(const Config& config);

    bool shouldTrigger(const std::string& sentence) const override;

protected:
    bool hasPunctuation(const std::string& sentence) const override;
    bool hasQuestionPattern(const std::string& sentence) const override;
};

/**
 * @brief Response manager for caching and merging
 *
 * Handles multiple concurrent backend responses and provides
 * intelligent caching and merging strategies.
 */
class AiChatClient::ResponseManager {
public:
    explicit ResponseManager(uint32_t maxConcurrentCalls);

    void addPendingRequest(const std::string& requestId, const std::string& conversation);
    void handleResponse(const std::string& requestId, const std::string& response);
    void invalidateOldRequests();

    bool hasCachedResponse() const;
    std::string getMergedResponse() const;

    void clear();
    size_t getPendingCount() const;
    bool hasPendingConversation(const std::string& conversation) const;

private:
    struct RequestInfo {
        std::string conversation;
        std::string response;
        std::chrono::steady_clock::time_point timestamp;
        bool isComplete;
    };

    std::unordered_map<std::string, RequestInfo> mRequests;
    uint32_t mMaxConcurrentCalls;

    bool isRequestExpired(const RequestInfo& request) const;
    static const uint32_t REQUEST_TIMEOUT_MS = 10000; // 10 seconds
};

/**
 * @brief Conversation state tracker
 *
 * Tracks conversation progress and completion status for
 * state management and optimization decisions.
 */
class AiChatClient::ConversationState {
public:
    enum class State {
        Idle,
        Accumulating,
        Processing,
        WaitingForEnd,
        Completed
    };

    ConversationState();

    void setState(State state);
    State getState() const;

    void markConversationStart();
    void markConversationEnd();
    void markProcessingStart();
    void markProcessingComplete();

    bool isProcessing() const;
    bool canAcceptSentences() const;

    void reset();

private:
    State mCurrentState;
};

} // namespace aichat
