#pragma once

#include <cstdint>
#include <string>

namespace edgeprocessor {

/**
 * @brief Configuration for AudioStreaming module
 *
 * Contains all necessary parameters for audio streaming including
 * audio format specifications and session management.
 */
struct AudioStreamingConfig {
    uint32_t sampleRateHz = 16000;        ///< Audio sample rate in Hz
    uint8_t bitsPerSample = 16;           ///< Bits per audio sample
    uint8_t channels = 1;                 ///< Number of audio channels
    std::string sessionId;                ///< Session identifier (auto-generated if empty)
    size_t chunkDurationMs = 20;          ///< Duration of each audio chunk in milliseconds
};

} // namespace edgeprocessor
