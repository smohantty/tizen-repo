#pragma once

#include "AudioStreamingConfig.h"
#include "IAudioStreamingListener.h"
#include <memory>
#include <functional>

namespace edgeprocessor {

// Forward declaration for transport adapter
class ITransportAdapter;

/**
 * @brief Main AudioStreaming class for Edge ASR
 *
 * Provides a simple API to stream audio to an Edge ASR service.
 * Handles buffering, sequencing, session lifecycle, and JSON message formatting.
 */
class AudioStreaming {
public:
    /**
     * @brief Constructor
     * @param config Audio streaming configuration
     * @param listener Callback interface for receiving results and events
     * @param transportAdapter External transport adapter for sending/receiving JSON messages
     */
    explicit AudioStreaming(AudioStreamingConfig config,
                           std::shared_ptr<IAudioStreamingListener> listener,
                           std::shared_ptr<ITransportAdapter> transportAdapter);

    /**
     * @brief Destructor
     */
    ~AudioStreaming();

    /**
     * @brief Copy constructor (deleted)
     */
    AudioStreaming(const AudioStreaming&) = delete;

    /**
     * @brief Move constructor (deleted)
     */
    AudioStreaming(AudioStreaming&&) = delete;

    /**
     * @brief Copy assignment operator (deleted)
     */
    AudioStreaming& operator=(const AudioStreaming&) = delete;

    /**
     * @brief Move assignment operator (deleted)
     */
    AudioStreaming& operator=(AudioStreaming&&) = delete;

    /**
     * @brief Start a new streaming session
     *
     * Sends a start message to the ASR service and transitions to Ready state.
     * Thread-safe and non-blocking.
     */
    void start();

    /**
     * @brief Continue streaming with PCM audio data
     * @param data Pointer to PCM audio data
     * @param bytes Size of audio data in bytes
     * @param ptsMs Presentation timestamp in milliseconds
     *
     * Buffers the audio data and sends it to the ASR service in chunks.
     * Thread-safe and non-blocking.
     */
    void continueWithPcm(const void* data, size_t bytes, uint64_t ptsMs);

    /**
     * @brief End the streaming session
     *
     * Sends an end message to the ASR service and transitions to Ending state.
     * Thread-safe and non-blocking.
     */
    void end();

    /**
     * @brief Cancel the streaming session
     *
     * Immediately cancels the session and transitions to Closed state.
     * Thread-safe and non-blocking.
     */
    void cancel();

    /**
     * @brief Get the current session ID
     * @return Session ID string
     */
    std::string getSessionId() const;

    /**
     * @brief Check if the streaming session is active
     * @return true if session is active (not in Idle or Closed state)
     */
    bool isActive() const;

    /**
     * @brief Get the current state as a string
     * @return State name (Idle, Ready, Streaming, Ending, Closed)
     */
    std::string getState() const;

private:
    class Impl;
    std::unique_ptr<Impl> mPimpl;
};

/**
 * @brief Interface for external transport adapter
 *
 * Implement this interface to provide transport functionality for
 * sending and receiving JSON messages with the Edge ASR service.
 */
class ITransportAdapter {
public:
    virtual ~ITransportAdapter() = default;

    /**
     * @brief Send a JSON message to the ASR service
     * @param jsonMessage JSON string to send
     * @return true if message was sent successfully
     */
    virtual bool send(const std::string& jsonMessage) = 0;

    /**
     * @brief Set callback for receiving JSON messages
     * @param callback Function to call when a message is received
     */
    virtual void setReceiveCallback(std::function<void(const std::string&)> callback) = 0;

    /**
     * @brief Check if the transport connection is active
     * @return true if connection is active
     */
    virtual bool isConnected() const = 0;
};

} // namespace edgeprocessor
