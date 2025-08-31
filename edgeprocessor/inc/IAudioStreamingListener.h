#pragma once

#include <string>

namespace edgeprocessor {

/**
 * @brief Interface for receiving AudioStreaming callbacks
 *
 * Implement this interface to receive notifications about
 * streaming events, results, and status updates.
 */
class IAudioStreamingListener {
public:
    virtual ~IAudioStreamingListener() = default;

    /**
     * @brief Called when the streaming session is ready to receive audio
     */
    virtual void onReady() = 0;

    /**
     * @brief Called when a partial ASR result is available
     * @param text The partial transcription text
     * @param stability Confidence level of the partial result (0.0 to 1.0)
     */
    virtual void onPartialResult(const std::string& text, float stability) = 0;

    /**
     * @brief Called when a final ASR result is available
     * @param text The final transcription text
     * @param confidence Confidence level of the final result (0.0 to 1.0)
     */
    virtual void onFinalResult(const std::string& text, float confidence) = 0;

    /**
     * @brief Called when latency information is received
     * @param upstreamMs Upstream latency in milliseconds
     * @param e2eMs End-to-end latency in milliseconds
     */
    virtual void onLatency(uint32_t upstreamMs, uint32_t e2eMs) = 0;

    /**
     * @brief Called when status information is received
     * @param message Status message from the ASR service
     */
    virtual void onStatus(const std::string& message) = 0;

    /**
     * @brief Called when an error occurs
     * @param error Error message describing what went wrong
     */
    virtual void onError(const std::string& error) = 0;

    /**
     * @brief Called when the streaming session is closed
     */
    virtual void onClosed() = 0;
};

} // namespace edgeprocessor
