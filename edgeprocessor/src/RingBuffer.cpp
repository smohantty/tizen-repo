#include "RingBuffer.h"
#include <algorithm>
#include <cstring>
#include <mutex>

namespace edgeprocessor {

class RingBuffer::Impl {
public:
    explicit Impl(size_t size)
        : mBuffer(size)
        , mSize(size)
        , mReadPos(0)
        , mWritePos(0)
        , mDataSize(0) {
    }

    size_t write(const void* data, size_t size) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (size == 0) return 0;

        size_t available = mSize - mDataSize;
        size_t toWrite = std::min(size, available);

        if (toWrite == 0) return 0;

        const uint8_t* bytes = static_cast<const uint8_t*>(data);

        // Write data in two parts if it wraps around the buffer
        size_t firstPart = std::min(toWrite, mSize - mWritePos);
        std::memcpy(&mBuffer[mWritePos], bytes, firstPart);

        if (firstPart < toWrite) {
            // Write remaining data at the beginning of the buffer
            std::memcpy(&mBuffer[0], bytes + firstPart, toWrite - firstPart);
        }

        mWritePos = (mWritePos + toWrite) % mSize;
        mDataSize += toWrite;

        return toWrite;
    }

    size_t read(void* data, size_t size) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (size == 0) return 0;

        size_t toRead = std::min(size, mDataSize);

        if (toRead == 0) return 0;

        uint8_t* bytes = static_cast<uint8_t*>(data);

        // Read data in two parts if it wraps around the buffer
        size_t firstPart = std::min(toRead, mSize - mReadPos);
        std::memcpy(bytes, &mBuffer[mReadPos], firstPart);

        if (firstPart < toRead) {
            // Read remaining data from the beginning of the buffer
            std::memcpy(bytes + firstPart, &mBuffer[0], toRead - firstPart);
        }

        mReadPos = (mReadPos + toRead) % mSize;
        mDataSize -= toRead;

        return toRead;
    }

    size_t availableRead() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mDataSize;
    }

    size_t availableWrite() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mSize - mDataSize;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mMutex);
        mReadPos = 0;
        mWritePos = 0;
        mDataSize = 0;
    }

    size_t size() const {
        return mSize;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mDataSize == 0;
    }

    bool full() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mDataSize == mSize;
    }

private:
    std::vector<uint8_t> mBuffer;
    size_t mSize;
    size_t mReadPos;
    size_t mWritePos;
    size_t mDataSize;
    mutable std::mutex mMutex;
};

// RingBuffer implementation
RingBuffer::RingBuffer(size_t size)
    : mPimpl(std::make_unique<Impl>(size)) {
}

RingBuffer::~RingBuffer() = default;

size_t RingBuffer::write(const void* data, size_t size) {
    return mPimpl->write(data, size);
}

size_t RingBuffer::read(void* data, size_t size) {
    return mPimpl->read(data, size);
}

size_t RingBuffer::availableRead() const {
    return mPimpl->availableRead();
}

size_t RingBuffer::availableWrite() const {
    return mPimpl->availableWrite();
}

void RingBuffer::clear() {
    mPimpl->clear();
}

size_t RingBuffer::size() const {
    return mPimpl->size();
}

bool RingBuffer::empty() const {
    return mPimpl->empty();
}

bool RingBuffer::full() const {
    return mPimpl->full();
}

} // namespace edgeprocessor
