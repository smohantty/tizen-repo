#pragma once
#include <memory>
#include <vector>
#include <functional>

namespace vad {

/**
 * @brief Speech state enumeration for streaming ASR
 */
enum class SpeechState {
    START,           ///< Speech has just started - initialize ASR
    CONTINUE,        ///< Speech is ongoing - continue feeding ASR
    END,             ///< Speech has ended - finalize ASR
    CONVERSATION_END ///< Entire conversation has ended after prolonged silence
};

/**
 * @brief Voice Activity Detection system using TensorFlow Lite model
 *
 * This class uses a TensorFlow Lite model to detect speech activity from
 * audio samples and provides callbacks with preroll buffers for ASR processing.
 */
class VoiceActivityDetector {
public:
    /**
     * @brief Callback function type for speech events
     * @param speechState current speech state (START, CONTINUE, END, CONVERSATION_END)
     * @param audioBuffer audio buffer - preroll buffer for START, current buffer for CONTINUE, empty for END and CONVERSATION_END
     * @param timestamp relative timestamp in milliseconds from start of processing
     */
    using SpeechEventCallback = std::function<void(SpeechState speechState, const std::vector<short>& audioBuffer, uint64_t timestamp)>;

    /**
     * @brief Constructor
     * @param modelPath Path to the TensorFlow Lite model file
     * @param sampleRate Audio sample rate in Hz (default: 16000)
     * Note: Frame size is fixed at 160 samples (10ms at 16kHz) as required by TensorFlow Lite model
     */
    explicit VoiceActivityDetector(const std::string& modelPath, int sampleRate = 16000);

    /**
     * @brief Destructor
     */
    ~VoiceActivityDetector();

    /**
     * @brief Process audio samples and update voice activity state
     * @param audiobuff Audio samples (16-bit signed integers)
     */
    void process(std::vector<short> audiobuff);

    /**
     * @brief Check if speech is currently active
     * @return true if speech is detected, false otherwise
     */
    bool isSpeechActive() const;

    /**
     * @brief Set callback function for speech events
     * @param callback Function to call when speech starts or stops
     */
    void setSpeechEventCallback(const SpeechEventCallback& callback);

    /**
     * @brief Reset detector state
     */
    void reset();

    /**
     * @brief Set speech probability threshold (0.0 to 1.0)
     * @param threshold Probability threshold for speech detection (default: 0.5)
     */
    void setSpeechThreshold(float threshold);

    /**
     * @brief Set minimum speech duration to trigger speech start event
     * @param durationMs Minimum duration in milliseconds (default: 100ms)
     */
    void setMinSpeechDuration(uint64_t durationMs);

    /**
     * @brief Set minimum silence duration to trigger speech end event
     * @param durationMs Minimum duration in milliseconds (default: 200ms)
     */
    void setMinSilenceDuration(uint64_t durationMs);

    /**
     * @brief Set conversation timeout duration to trigger conversation end event
     * @param durationMs Duration in milliseconds after last speech end to trigger conversation end (default: 2000ms)
     */
    void setConversationTimeout(uint64_t durationMs);

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace vad