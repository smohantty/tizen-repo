#include "RingBuffer.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace utils {

template<typename T>
class RingBuffer<T>::Impl {
public:
    explicit Impl(std::size_t capacity)
        : mCapacity(capacity)
        , mSize(0)
        , mHead(0)
        , mTail(0) {
        if (capacity == 0) {
            throw std::invalid_argument("RingBuffer capacity must be greater than 0");
        }
        mBuffer = std::make_unique<T[]>(capacity);
    }

    ~Impl() = default;

    Impl(const Impl& other)
        : mCapacity(other.mCapacity)
        , mSize(other.mSize)
        , mHead(0)
        , mTail(0) {
        mBuffer = std::make_unique<T[]>(mCapacity);
        if (mSize > 0) {
            // Copy elements in order, starting from head
            std::size_t srcIndex = other.mHead;
            for (std::size_t i = 0; i < mSize; ++i) {
                mBuffer[i] = other.mBuffer[srcIndex];
                srcIndex = (srcIndex + 1) % other.mCapacity;
            }
            mTail = mSize;
        }
    }

    Impl& operator=(const Impl& other) {
        if (this != &other) {
            mCapacity = other.mCapacity;
            mSize = other.mSize;
            mHead = 0;
            mTail = 0;

            mBuffer = std::make_unique<T[]>(mCapacity);
            if (mSize > 0) {
                // Copy elements in order, starting from head
                std::size_t srcIndex = other.mHead;
                for (std::size_t i = 0; i < mSize; ++i) {
                    mBuffer[i] = other.mBuffer[srcIndex];
                    srcIndex = (srcIndex + 1) % other.mCapacity;
                }
                mTail = mSize;
            }
        }
        return *this;
    }

    bool push(const T& value) {
        if (full()) {
            return false;
        }

        mBuffer[mTail] = value;
        mTail = (mTail + 1) % mCapacity;
        mSize++;
        return true;
    }

    bool push(T&& value) {
        if (full()) {
            return false;
        }

        mBuffer[mTail] = std::move(value);
        mTail = (mTail + 1) % mCapacity;
        mSize++;
        return true;
    }

    bool pop(T& value) {
        if (empty()) {
            return false;
        }

        value = std::move(mBuffer[mHead]);
        mHead = (mHead + 1) % mCapacity;
        mSize--;
        return true;
    }

    bool front(T& value) const {
        if (empty()) {
            return false;
        }

        value = mBuffer[mHead];
        return true;
    }

    bool empty() const {
        return mSize == 0;
    }

    bool full() const {
        return mSize == mCapacity;
    }

    std::size_t size() const {
        return mSize;
    }

    std::size_t capacity() const {
        return mCapacity;
    }

    void clear() {
        mSize = 0;
        mHead = 0;
        mTail = 0;
    }

private:
    std::unique_ptr<T[]> mBuffer;
    std::size_t mCapacity;
    std::size_t mSize;
    std::size_t mHead;
    std::size_t mTail;
};

// Template method implementations
template<typename T>
RingBuffer<T>::RingBuffer(std::size_t capacity)
    : mPimpl(std::make_unique<Impl>(capacity)) {
}




template<typename T>
RingBuffer<T>::~RingBuffer() = default;

template<typename T>
bool RingBuffer<T>::push(const T& value) {
    return mPimpl->push(value);
}

template<typename T>
bool RingBuffer<T>::push(T&& value) {
    return mPimpl->push(std::move(value));
}

template<typename T>
bool RingBuffer<T>::pop(T& value) {
    return mPimpl->pop(value);
}

template<typename T>
bool RingBuffer<T>::front(T& value) const {
    return mPimpl->front(value);
}

template<typename T>
bool RingBuffer<T>::empty() const {
    return mPimpl->empty();
}

template<typename T>
bool RingBuffer<T>::full() const {
    return mPimpl->full();
}

template<typename T>
std::size_t RingBuffer<T>::size() const {
    return mPimpl->size();
}

template<typename T>
std::size_t RingBuffer<T>::capacity() const {
    return mPimpl->capacity();
}

template<typename T>
void RingBuffer<T>::clear() {
    mPimpl->clear();
}

// Explicit template instantiations for common types
template class RingBuffer<int>;
template class RingBuffer<double>;
template class RingBuffer<std::string>;

} // namespace utils
