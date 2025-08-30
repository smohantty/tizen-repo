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
    // Pimpl idiom - hide all implementation details
    class Impl;
    std::unique_ptr<Impl> mPImpl;
};

} // namespace aichat
