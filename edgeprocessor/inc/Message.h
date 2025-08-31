#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <variant>

namespace edgeprocessor {

// Forward declarations for internal message types
struct CmdStart;
struct CmdContinue;
struct CmdEnd;
struct CmdCancel;
struct EvPartial;
struct EvFinal;
struct EvLatency;
struct EvStatus;
struct EvError;
struct EvClosed;

/**
 * @brief Command to start a new streaming session
 */
struct CmdStart {
    // Empty struct - start command has no additional data
};

/**
 * @brief Command to continue streaming with PCM audio data
 */
struct CmdContinue {
    std::vector<uint8_t> pcm;     ///< PCM audio data
    uint64_t ptsMs;               ///< Presentation timestamp in milliseconds
};

/**
 * @brief Command to end the streaming session
 */
struct CmdEnd {
    // Empty struct - end command has no additional data
};

/**
 * @brief Command to cancel the streaming session
 */
struct CmdCancel {
    // Empty struct - cancel command has no additional data
};

/**
 * @brief Event for partial ASR result
 */
struct EvPartial {
    std::string text;             ///< Partial transcription text
    float stability;              ///< Stability/confidence (0.0 to 1.0)
};

/**
 * @brief Event for final ASR result
 */
struct EvFinal {
    std::string text;             ///< Final transcription text
    float confidence;             ///< Confidence level (0.0 to 1.0)
};

/**
 * @brief Event for latency information
 */
struct EvLatency {
    uint32_t upstreamMs;          ///< Upstream latency in milliseconds
    uint32_t e2eMs;               ///< End-to-end latency in milliseconds
};

/**
 * @brief Event for status information
 */
struct EvStatus {
    std::string message;          ///< Status message
};

/**
 * @brief Event for error conditions
 */
struct EvError {
    std::string what;             ///< Error description
};

/**
 * @brief Event for session closure
 */
struct EvClosed {
    // Empty struct - closed event has no additional data
};

/**
 * @brief Variant type for all internal messages
 *
 * This variant contains all possible command and event types
 * that can be passed between components of the AudioStreaming module.
 */
using Message = std::variant<CmdStart, CmdContinue, CmdEnd, CmdCancel,
                            EvPartial, EvFinal, EvLatency, EvStatus, EvError, EvClosed>;

} // namespace edgeprocessor
