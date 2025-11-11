#ifndef WAVE_HEADER_H
#define WAVE_HEADER_H

#include <cstdint>
#include <cstring>
#include <string>

namespace utils {

/**
 * @brief WAV file header structure for standard PCM format
 *
 * This structure can be directly read from or written to a buffer.
 * Total size: 44 bytes
 */
#pragma pack(push, 1)
struct WaveHeader {
    // RIFF Header (12 bytes)
    char mRiffId[4];        // "RIFF"
    uint32_t mFileSize;     // File size - 8
    char mWaveId[4];        // "WAVE"

    // Format Chunk (24 bytes)
    char mFmtId[4];         // "fmt "
    uint32_t mFmtSize;      // Size of format chunk (16 for PCM)
    uint16_t mAudioFormat;  // Audio format (1 = PCM)
    uint16_t mNumChannels;  // Number of channels
    uint32_t mSampleRate;   // Sample rate in Hz
    uint32_t mByteRate;     // Byte rate (SampleRate * NumChannels * BitsPerSample/8)
    uint16_t mBlockAlign;   // Block align (NumChannels * BitsPerSample/8)
    uint16_t mBitsPerSample;// Bits per sample

    // Data Chunk (8 bytes)
    char mDataId[4];        // "data"
    uint32_t mDataSize;     // Size of audio data in bytes

    /**
     * @brief Construct a new Wave Header with default or specified parameters
     */
    WaveHeader(uint16_t numChannels = 1,
               uint32_t sampleRate = 16000,
               uint16_t bitsPerSample = 16,
               uint32_t numSamples = 0);

    /**
     * @brief Read WAV header from a buffer
     *
     * @param buffer Pointer to buffer containing WAV header (at least 44 bytes)
     * @return true if successfully read and valid
     */
    bool readFromBuffer(const char* buffer);

    /**
     * @brief Write WAV header to a buffer
     *
     * @param buffer Pointer to buffer (must be at least 44 bytes)
     */
    void writeToBuffer(char* buffer) const;

    /**
     * @brief Read WAV header from a file
     *
     * @param filename Path to WAV file
     * @return true if successful
     */
    bool readFromFile(const std::string& filename);

    /**
     * @brief Write WAV header to a file (creates new file or overwrites)
     *
     * @param filename Path to output WAV file
     * @return true if successful
     */
    bool writeToFile(const std::string& filename) const;

    /**
     * @brief Validate if the header represents a valid WAV format
     *
     * @return true if valid
     */
    bool isValid() const;

    /**
     * @brief Update computed fields based on current parameters
     *
     * Updates mByteRate, mBlockAlign, mDataSize, and mFileSize
     */
    void updateComputedFields();

    /**
     * @brief Get number of samples per channel
     *
     * @return uint32_t Number of samples
     */
    uint32_t getNumSamples() const;

    /**
     * @brief Set number of samples and update data size
     *
     * @param numSamples Number of samples per channel
     */
    void setNumSamples(uint32_t numSamples);

    /**
     * @brief Get duration in seconds
     *
     * @return double Duration in seconds
     */
    double getDuration() const;

    /**
     * @brief Get a human-readable description of the WAV format
     *
     * @return std::string Description string
     */
    std::string getDescription() const;
};
#pragma pack(pop)

static_assert(sizeof(WaveHeader) == 44, "WaveHeader must be exactly 44 bytes");

} // namespace utils

#endif // WAVE_HEADER_H
