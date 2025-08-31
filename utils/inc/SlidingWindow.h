#pragma once

#include <vector>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace utils {

template<typename T, std::size_t FrameSize>
class SlidingWindow {
public:
    static_assert(FrameSize > 0, "Frame size must be greater than 0");

    using size_type = std::size_t;
    static constexpr size_type frame_size = FrameSize;

    /**
     * @brief Constructor
     * @param windowSize Maximum number of frames the window can hold
     */
    constexpr SlidingWindow(size_type windowSize)
        : mWindowSize(windowSize)
        , mTotalFramesAdded(0)
        , mTotalFramesDropped(0) {
        if (windowSize == 0) {
            throw std::invalid_argument("Window size must be greater than 0");
        }

        mData.reserve(windowSize * FrameSize);
    }

    /**
     * @brief Destructor
     */
    ~SlidingWindow() = default;

    // Move operations are defaulted
    constexpr SlidingWindow(SlidingWindow&&) = default;
    constexpr SlidingWindow& operator=(SlidingWindow&&) = default;

    /**
     * @brief Add a frame to the window
     * @param frame Vector of items (must match frame size)
     */
    constexpr void addFrame(const std::vector<T>& frame) {
        if (frame.size() != FrameSize) {
            throw std::invalid_argument("Frame size mismatch: expected " +
                                      std::to_string(FrameSize) + ", got " +
                                      std::to_string(frame.size()));
        }

        if (isFull()) {
            // Drop oldest frame by removing first FrameSize elements
            mData.erase(mData.begin(), mData.begin() + FrameSize);
            mTotalFramesDropped++;
        }

        // Add new frame data
        mData.insert(mData.end(), frame.begin(), frame.end());
        mTotalFramesAdded++;
    }

    /**
     * @brief Add a frame from raw data
     * @param data Pointer to data
     * @param count Number of items (must match frame size)
     */
    constexpr void addFrame(const T* data, size_type count) {
        if (count != FrameSize) {
            throw std::invalid_argument("Frame size mismatch: expected " +
                                      std::to_string(FrameSize) + ", got " +
                                      std::to_string(count));
        }

        if (isFull()) {
            // Drop oldest frame by removing first FrameSize elements
            mData.erase(mData.begin(), mData.begin() + FrameSize);
            mTotalFramesDropped++;
        }

        // Add new frame data
        mData.insert(mData.end(), data, data + count);
        mTotalFramesAdded++;
    }

    /**
     * @brief Add a frame from std::array (compile-time size check)
     * @param frame Array of items
     */
    template<std::size_t N>
    constexpr void addFrame(const std::array<T, N>& frame) {
        static_assert(N == FrameSize, "Frame size mismatch");

        if (isFull()) {
            // Drop oldest frame by removing first FrameSize elements
            mData.erase(mData.begin(), mData.begin() + FrameSize);
            mTotalFramesDropped++;
        }

        // Add new frame data
        mData.insert(mData.end(), frame.begin(), frame.end());
        mTotalFramesAdded++;
    }

    /**
     * @brief Add a single item frame (convenience for frameSize=1)
     * @param item Single item to add
     */
    constexpr void addFrame(const T& item) {
        static_assert(FrameSize == 1, "Single item add only allowed when frame size is 1");

        if (isFull()) {
            // Drop oldest item
            mData.erase(mData.begin());
            mTotalFramesDropped++;
        }

        mData.push_back(item);
        mTotalFramesAdded++;
    }

    /**
     * @brief Get all data in the window
     * @return Reference to the flat data vector
     */
    constexpr const std::vector<T>& getData() const noexcept {
        return mData;
    }

    /**
     * @brief Get a specific frame by index
     * @param index Frame index (0 = oldest, getFrameCount()-1 = newest)
     * @return Vector containing the frame data
     */
    constexpr std::vector<T> getFrame(size_type index) const {
        if (index >= getFrameCount()) {
            throw std::out_of_range("Frame index out of range");
        }

        size_type startIndex = index * FrameSize;
        return std::vector<T>(mData.begin() + startIndex,
                             mData.begin() + startIndex + FrameSize);
    }

    /**
     * @brief Get the newest frame
     * @return Vector containing the newest frame data
     */
    constexpr std::vector<T> getLatestFrame() const {
        if (isEmpty()) {
            throw std::out_of_range("No frames available");
        }

        size_type startIndex = mData.size() - FrameSize;
        return std::vector<T>(mData.begin() + startIndex, mData.end());
    }

    /**
     * @brief Get a view of a specific frame (no copy)
     * @param index Frame index (0 = oldest, getFrameCount()-1 = newest)
     * @return Pair of iterators representing the frame range
     */
    constexpr std::pair<typename std::vector<T>::const_iterator,
                       typename std::vector<T>::const_iterator>
    getFrameView(size_type index) const {
        if (index >= getFrameCount()) {
            throw std::out_of_range("Frame index out of range");
        }

        size_type startIndex = index * FrameSize;
        return {mData.begin() + startIndex, mData.begin() + startIndex + FrameSize};
    }

    /**
     * @brief Get current number of frames in window
     * @return Number of frames
     */
    constexpr size_type getFrameCount() const noexcept {
        return mData.size() / FrameSize;
    }

    /**
     * @brief Get maximum window size
     * @return Maximum number of frames
     */
    constexpr size_type getWindowSize() const noexcept {
        return mWindowSize;
    }

    /**
     * @brief Get frame size (compile-time constant)
     * @return Frame size
     */
    static constexpr size_type getFrameSize() noexcept {
        return FrameSize;
    }

    /**
     * @brief Check if window is full
     * @return true if window is full
     */
    constexpr bool isFull() const noexcept {
        return getFrameCount() >= mWindowSize;
    }

    /**
     * @brief Check if window is empty
     * @return true if window is empty
     */
    constexpr bool isEmpty() const noexcept {
        return mData.empty();
    }

    /**
     * @brief Get total number of items across all frames
     * @return Total items
     */
    constexpr size_type getTotalItems() const noexcept {
        return mData.size();
    }

    /**
     * @brief Get total frames added since creation
     * @return Total frames added
     */
    constexpr size_type getTotalFramesAdded() const noexcept {
        return mTotalFramesAdded;
    }

    /**
     * @brief Get total frames dropped since creation
     * @return Total frames dropped
     */
    constexpr size_type getTotalFramesDropped() const noexcept {
        return mTotalFramesDropped;
    }

    /**
     * @brief Get drop rate (dropped / (added + dropped))
     * @return Drop rate as percentage
     */
    constexpr double getDropRate() const noexcept {
        size_type total = mTotalFramesAdded + mTotalFramesDropped;
        return total > 0 ? static_cast<double>(mTotalFramesDropped) / total : 0.0;
    }

    /**
     * @brief Clear all frames from window
     */
    constexpr void clear() noexcept {
        mData.clear();
    }

    /**
     * @brief Reset window and statistics
     */
    constexpr void reset() noexcept {
        mData.clear();
        mTotalFramesAdded = 0;
        mTotalFramesDropped = 0;
    }

    // Direct access to data (for algorithms)
    constexpr const T* data() const noexcept { return mData.data(); }
    constexpr size_type size() const noexcept { return mData.size(); }

    // Iterators for the flat data
    constexpr auto begin() const noexcept { return mData.begin(); }
    constexpr auto end() const noexcept { return mData.end(); }
    constexpr auto cbegin() const noexcept { return mData.cbegin(); }
    constexpr auto cend() const noexcept { return mData.cend(); }
    constexpr auto rbegin() const noexcept { return mData.rbegin(); }
    constexpr auto rend() const noexcept { return mData.rend(); }
    constexpr auto crbegin() const noexcept { return mData.crbegin(); }
    constexpr auto crend() const noexcept { return mData.crend(); }

private:
    std::vector<T> mData;
    size_type mWindowSize;
    size_type mTotalFramesAdded;
    size_type mTotalFramesDropped;
};

} // namespace utils
