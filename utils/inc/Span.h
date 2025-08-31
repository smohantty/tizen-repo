#pragma once

#include <cstddef>
#include <type_traits>
#include <array>
#include <vector>
#include <iterator>
#include <stdexcept>

namespace utils {

// C++17 equivalent of std::dynamic_extent
constexpr std::size_t dynamic_extent = std::size_t(-1);

template<typename T>
class Span {
public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /**
     * @brief Default constructor
     */
    constexpr Span() noexcept : mData(nullptr), mSize(0) {}

    /**
     * @brief Constructor from pointer and size
     * @param ptr Pointer to the first element
     * @param count Number of elements
     */
    constexpr Span(pointer ptr, size_type count) noexcept : mData(ptr), mSize(count) {}

    /**
     * @brief Constructor from array
     * @param arr Reference to array
     */
    template<std::size_t N>
    constexpr Span(element_type (&arr)[N]) noexcept : mData(arr), mSize(N) {}

    /**
     * @brief Constructor from std::array
     * @param arr Reference to std::array
     */
    template<std::size_t N>
    constexpr Span(std::array<value_type, N>& arr) noexcept : mData(arr.data()), mSize(N) {}

    /**
     * @brief Constructor from const std::array
     * @param arr Reference to const std::array
     */
    template<std::size_t N>
    constexpr Span(const std::array<value_type, N>& arr) noexcept : mData(const_cast<T*>(arr.data())), mSize(N) {}

    /**
     * @brief Constructor from vector
     * @param vec Reference to vector
     */
    template<typename Allocator>
    constexpr Span(std::vector<value_type, Allocator>& vec) noexcept : mData(vec.data()), mSize(vec.size()) {}

    /**
     * @brief Constructor from const vector
     * @param vec Reference to const vector
     */
    template<typename Allocator>
    constexpr Span(const std::vector<value_type, Allocator>& vec) noexcept : mData(const_cast<T*>(vec.data())), mSize(vec.size()) {}

    /**
     * @brief Destructor
     */
    ~Span() = default;

    // Move operations are defaulted
    constexpr Span(Span&&) = default;
    constexpr Span& operator=(Span&&) = default;

    /**
     * @brief Access element at index
     * @param idx Index of element
     * @return Reference to element
     */
    constexpr reference operator[](size_type idx) const {
        if (idx >= mSize) {
            throw std::out_of_range("Span index out of range");
        }
        return mData[idx];
    }

    /**
     * @brief Access first element
     * @return Reference to first element
     */
    constexpr reference front() const {
        if (mSize == 0) {
            throw std::out_of_range("Span is empty");
        }
        return mData[0];
    }

    /**
     * @brief Access last element
     * @return Reference to last element
     */
    constexpr reference back() const {
        if (mSize == 0) {
            throw std::out_of_range("Span is empty");
        }
        return mData[mSize - 1];
    }

    /**
     * @brief Get pointer to data
     * @return Pointer to first element
     */
    constexpr pointer data() const noexcept {
        return mData;
    }

    /**
     * @brief Get size of span
     * @return Number of elements
     */
    constexpr size_type size() const noexcept {
        return mSize;
    }

    /**
     * @brief Get size in bytes
     * @return Size in bytes
     */
    constexpr size_type size_bytes() const noexcept {
        return mSize * sizeof(T);
    }

    /**
     * @brief Check if span is empty
     * @return true if empty
     */
    constexpr bool empty() const noexcept {
        return mSize == 0;
    }

    /**
     * @brief Get iterator to beginning
     * @return Iterator to first element
     */
    constexpr iterator begin() const noexcept {
        return mData;
    }

    /**
     * @brief Get iterator to end
     * @return Iterator past last element
     */
    constexpr iterator end() const noexcept {
        return mData + mSize;
    }

    /**
     * @brief Get const iterator to beginning
     * @return Const iterator to first element
     */
    constexpr const_iterator cbegin() const noexcept {
        return mData;
    }

    /**
     * @brief Get const iterator to end
     * @return Const iterator past last element
     */
    constexpr const_iterator cend() const noexcept {
        return mData + mSize;
    }

    /**
     * @brief Get reverse iterator to beginning
     * @return Reverse iterator to last element
     */
    constexpr reverse_iterator rbegin() const noexcept {
        return reverse_iterator(end());
    }

    /**
     * @brief Get reverse iterator to end
     * @return Reverse iterator past first element
     */
    constexpr reverse_iterator rend() const noexcept {
        return reverse_iterator(begin());
    }

    /**
     * @brief Get const reverse iterator to beginning
     * @return Const reverse iterator to last element
     */
    constexpr const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    /**
     * @brief Get const reverse iterator to end
     * @return Const reverse iterator past first element
     */
    constexpr const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    /**
     * @brief Create subspan
     * @param offset Starting offset
     * @param count Number of elements (default: to end)
     * @return New span
     */
    constexpr Span<T> subspan(size_type offset, size_type count = dynamic_extent) const {
        if (offset > mSize) {
            throw std::out_of_range("Span offset out of range");
        }

        if (count == dynamic_extent) {
            count = mSize - offset;
        } else if (offset + count > mSize) {
            throw std::out_of_range("Span count out of range");
        }

        return Span<T>(mData + offset, count);
    }

private:
    T* mData;
    std::size_t mSize;
};

// Specialization for const T
template<typename T>
using Span_const = Span<const T>;

} // namespace utils
