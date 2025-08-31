#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <cstddef>
#include <memory>

namespace utils {

template<typename T>
class RingBuffer {
public:
    /**
     * @brief Constructor
     * @param capacity Maximum number of elements the buffer can hold
     */
    explicit RingBuffer(std::size_t capacity);

    /**
     * @brief Destructor
     */
    ~RingBuffer();

    // Move operations are defaulted (unique_ptr supports move)
    RingBuffer(RingBuffer&&) = default;
    RingBuffer& operator=(RingBuffer&&) = default;

    /**
     * @brief Push an element to the buffer
     * @param value The value to push
     * @return true if successful, false if buffer is full
     */
    bool push(const T& value);

    /**
     * @brief Push an element to the buffer (move version)
     * @param value The value to push
     * @return true if successful, false if buffer is full
     */
    bool push(T&& value);

    /**
     * @brief Pop an element from the buffer
     * @param value Reference to store the popped value
     * @return true if successful, false if buffer is empty
     */
    bool pop(T& value);

    /**
     * @brief Get the front element without removing it
     * @param value Reference to store the front value
     * @return true if successful, false if buffer is empty
     */
    bool front(T& value) const;

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

    /**
     * @brief Get current number of elements in buffer
     * @return Number of elements
     */
    std::size_t size() const;

    /**
     * @brief Get maximum capacity of buffer
     * @return Maximum capacity
     */
    std::size_t capacity() const;

    /**
     * @brief Clear all elements from buffer
     */
    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> mPimpl;
};

} // namespace utils

#endif // RINGBUFFER_H
