#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace edgeprocessor {

/**
 * @brief Thread-safe ring buffer for PCM audio data
 *
 * Provides efficient buffering of audio data with configurable size.
 * Supports concurrent read/write operations with proper synchronization.
 */
class RingBuffer {
public:
    /**
     * @brief Constructor
     * @param size Buffer size in bytes
     */
    explicit RingBuffer(size_t size);

    /**
     * @brief Destructor
     */
    ~RingBuffer();

    /**
     * @brief Copy constructor (deleted)
     */
    RingBuffer(const RingBuffer&) = delete;

    /**
     * @brief Move constructor (deleted)
     */
    RingBuffer(RingBuffer&&) = delete;

    /**
     * @brief Copy assignment operator (deleted)
     */
    RingBuffer& operator=(const RingBuffer&) = delete;

    /**
     * @brief Move assignment operator (deleted)
     */
    RingBuffer& operator=(RingBuffer&&) = delete;

    /**
     * @brief Write data to the ring buffer
     * @param data Pointer to data to write
     * @param size Size of data in bytes
     * @return Number of bytes actually written
     */
    size_t write(const void* data, size_t size);

    /**
     * @brief Read data from the ring buffer
     * @param data Pointer to destination buffer
     * @param size Maximum number of bytes to read
     * @return Number of bytes actually read
     */
    size_t read(void* data, size_t size);

    /**
     * @brief Get the number of bytes available for reading
     * @return Number of bytes available
     */
    size_t availableRead() const;

    /**
     * @brief Get the number of bytes available for writing
     * @return Number of bytes available
     */
    size_t availableWrite() const;

    /**
     * @brief Clear the buffer
     */
    void clear();

    /**
     * @brief Get the total buffer size
     * @return Buffer size in bytes
     */
    size_t size() const;

    /**
     * @brief Check if buffer is empty
     * @return true if buffer is empty
     */
    bool empty() const;

    /**
     * @brief Check if buffer is full
     * @return true if buffer is full
     */
    bool full() const;

private:
    class Impl;
    std::unique_ptr<Impl> mPimpl;
};

} // namespace edgeprocessor
