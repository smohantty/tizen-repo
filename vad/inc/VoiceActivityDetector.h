#pragma once
#include <memory>
#include <vector>
#include <functional>

namespace vad {

/**
 * @brief Voice Activity Detection system that detects speech start and end
 * from continuous audio buffer streams.
 *
 * This class provides a simple API to process audio buffers and detect
 * when speech activity begins and ends.
 */
class VoiceActivityDetector {
public:
    /**
     * @brief Callback function type for speech events
     * @param isSpeechActive true when speech starts, false when speech ends
     * @param timestamp relative timestamp in milliseconds from start of processing
     */
    using SpeechEventCallback = std::function<void(bool isSpeechActive, uint64_t timestamp)>;

    /**
     * @brief Constructor with optional parameters
     * @param sampleRate Audio sample rate in Hz (default: 16000)
     * @param frameSize Number of samples per processing frame (default: 1600, which is 100ms at 16kHz)
     */
    explicit VoiceActivityDetector(int sampleRate = 16000, size_t frameSize = 1600);

    /**
     * @brief Destructor
     */
    ~VoiceActivityDetector();

    /**
     * @brief Process a buffer of audio samples
     * @param buffer Audio samples (16-bit signed integers)
     * @return true if voice activity state changed during processing
     */
    bool processAudioBuffer(const std::vector<short>& buffer);

    /**
     * @brief Check if speech is currently active
     * @return true if speech is detected, false otherwise
     */
    bool isSpeechActive() const;

    /**
     * @brief Get timestamp of last speech state change
     * @return timestamp in milliseconds from start of processing
     */
    uint64_t getLastStateChangeTimestamp() const;

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
     * @brief Set energy threshold for voice activity detection
     * @param threshold Energy threshold value (higher = less sensitive)
     */
    void setEnergyThreshold(double threshold);

    /**
     * @brief Set minimum speech duration to trigger speech start event
     * @param durationMs Minimum duration in milliseconds
     */
    void setMinSpeechDuration(uint64_t durationMs);

    /**
     * @brief Set minimum silence duration to trigger speech end event
     * @param durationMs Minimum duration in milliseconds
     */
    void setMinSilenceDuration(uint64_t durationMs);

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace vad
