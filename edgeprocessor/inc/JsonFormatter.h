#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include "../src/json.hpp"

namespace edgeprocessor {

// Forward declarations
struct AudioStreamingConfig;
struct CmdStart;
struct CmdContinue;
struct CmdEnd;
struct EvPartial;
struct EvFinal;
struct EvLatency;
struct EvStatus;
struct EvError;

/**
 * @brief JSON formatting utilities for AudioStreaming module
 *
 * Handles encoding of outbound messages and decoding of inbound messages
 * with support for Base64 encoding of audio data.
 */
class JsonFormatter {
public:
    /**
     * @brief Constructor
     */
    JsonFormatter();

    /**
     * @brief Destructor
     */
    ~JsonFormatter();

    /**
     * @brief Copy constructor (deleted)
     */
    JsonFormatter(const JsonFormatter&) = delete;

    /**
     * @brief Move constructor (deleted)
     */
    JsonFormatter(JsonFormatter&&) = delete;

    /**
     * @brief Copy assignment operator (deleted)
     */
    JsonFormatter& operator=(const JsonFormatter&) = delete;

    /**
     * @brief Move assignment operator (deleted)
     */
    JsonFormatter& operator=(JsonFormatter&&) = delete;

    /**
     * @brief Format start command to JSON
     * @param config Audio streaming configuration
     * @return JSON string representation
     */
    std::string formatStart(const AudioStreamingConfig& config);

    /**
     * @brief Format audio data command to JSON
     * @param pcmData PCM audio data
     * @param ptsMs Presentation timestamp in milliseconds
     * @param seq Sequence number
     * @param last Whether this is the last audio chunk
     * @return JSON string representation
     */
    std::string formatAudio(const std::vector<uint8_t>& pcmData,
                           uint64_t ptsMs,
                           uint32_t seq,
                           bool last = false);

    /**
     * @brief Format end command to JSON
     * @param seq Sequence number
     * @return JSON string representation
     */
    std::string formatEnd(uint32_t seq);

    /**
     * @brief Parse partial result event from JSON
     * @param json JSON string to parse
     * @return Parsed EvPartial struct
     * @throws std::runtime_error if parsing fails
     */
    EvPartial parsePartial(const std::string& json);

    /**
     * @brief Parse final result event from JSON
     * @param json JSON string to parse
     * @return Parsed EvFinal struct
     * @throws std::runtime_error if parsing fails
     */
    EvFinal parseFinal(const std::string& json);

    /**
     * @brief Parse latency event from JSON
     * @param json JSON string to parse
     * @return Parsed EvLatency struct
     * @throws std::runtime_error if parsing fails
     */
    EvLatency parseLatency(const std::string& json);

    /**
     * @brief Parse status event from JSON
     * @param json JSON string to parse
     * @return Parsed EvStatus struct
     * @throws std::runtime_error if parsing fails
     */
    EvStatus parseStatus(const std::string& json);

    /**
     * @brief Parse error event from JSON
     * @param json JSON string to parse
     * @return Parsed EvError struct
     * @throws std::runtime_error if parsing fails
     */
    EvError parseError(const std::string& json);

    /**
     * @brief Encode binary data to Base64 string
     * @param data Binary data to encode
     * @return Base64 encoded string
     */
    static std::string encodeBase64(const std::vector<uint8_t>& data);

    /**
     * @brief Decode Base64 string to binary data
     * @param base64 Base64 encoded string
     * @return Decoded binary data
     * @throws std::runtime_error if decoding fails
     */
    static std::vector<uint8_t> decodeBase64(const std::string& base64);

    /**
     * @brief Generate a UUID string
     * @return UUID string in format xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
     */
    static std::string generateUuid();

private:
    class Impl;
    std::unique_ptr<Impl> mPimpl;
};

} // namespace edgeprocessor
