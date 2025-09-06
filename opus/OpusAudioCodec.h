#pragma once

#include <vector>
#include <memory>
#include <opus.h>

namespace opus {

/**
 * @brief Opus audio codec for encoding/decoding PCM audio data
 *
 * This class provides functionality to compress raw PCM audio frames
 * (16 kHz, mono, 16-bit little-endian) into Opus-encoded data and
 * perform the reverse operation.
 */
class OpusAudioCodec {
public:
    /**
     * @brief Constructor
     * @param sampleRate Sample rate in Hz (default: 16000)
     * @param channels Number of channels (default: 1 for mono)
     * @param application Opus application type (default: OPUS_APPLICATION_VOIP)
     */
    OpusAudioCodec(int sampleRate = 16000, int channels = 1, int application = OPUS_APPLICATION_VOIP);

    /**
     * @brief Destructor
     */
    ~OpusAudioCodec();

    // Delete copy constructor and assignment operator
    OpusAudioCodec(const OpusAudioCodec&) = delete;
    OpusAudioCodec& operator=(const OpusAudioCodec&) = delete;

    /**
     * @brief Encode PCM samples to Opus compressed data
     * @param pcm Vector of PCM samples (opus_int16)
     * @return Vector of compressed Opus data
     * @throws std::runtime_error if encoding fails
     */
    std::vector<unsigned char> encode(const std::vector<opus_int16>& pcm);

    /**
     * @brief Decode Opus compressed data to PCM samples
     * @param data Vector of compressed Opus data
     * @return Vector of decoded PCM samples
     * @throws std::runtime_error if decoding fails
     */
    std::vector<opus_int16> decode(const std::vector<unsigned char>& data);

    /**
     * @brief Get the frame size in samples
     * @return Frame size in samples
     */
    int getFrameSize() const;

    /**
     * @brief Get the sample rate
     * @return Sample rate in Hz
     */
    int getSampleRate() const;

    /**
     * @brief Get the number of channels
     * @return Number of channels
     */
    int getChannels() const;

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace opus
