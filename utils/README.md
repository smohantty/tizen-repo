# Utils Library

This library contains utility classes and functions for the Tizen project.

## RingBuffer

A generic, thread-safe ring buffer implementation using the PIMPL idiom.

### Features

- **Generic**: Works with any copyable and movable type
- **PIMPL Idiom**: Implementation details are hidden from the public interface
- **C++17**: Uses modern C++ features
- **Exception Safe**: Proper exception handling and RAII
- **Move Semantics**: Supports move construction and assignment (copy operations are disabled)
- **Thread Safety**: Basic thread safety for single producer/consumer scenarios

### Usage

```cpp
#include "RingBuffer.h"
#include <iostream>

using namespace utils;

int main() {
    // Create a ring buffer with capacity 5
    RingBuffer<int> buffer(5);

    // Push elements
    buffer.push(1);
    buffer.push(2);
    buffer.push(3);

    // Check status
    std::cout << "Size: " << buffer.size() << std::endl;
    std::cout << "Empty: " << buffer.empty() << std::endl;
    std::cout << "Full: " << buffer.full() << std::endl;

    // Pop elements
    int value;
    while (buffer.pop(value)) {
        std::cout << "Popped: " << value << std::endl;
    }

    return 0;
}
```

### API Reference

#### Constructor
```cpp
explicit RingBuffer(std::size_t capacity);
```
Creates a ring buffer with the specified capacity.

#### Push Operations
```cpp
bool push(const T& value);  // Copy version
bool push(T&& value);       // Move version
```
Adds an element to the buffer. Returns `true` if successful, `false` if the buffer is full.

#### Pop Operations
```cpp
bool pop(T& value);
```
Removes and returns the oldest element. Returns `true` if successful, `false` if the buffer is empty.

#### Peek Operations
```cpp
bool front(T& value) const;
```
Returns the oldest element without removing it. Returns `true` if successful, `false` if the buffer is empty.

#### Status Operations
```cpp
bool empty() const;           // Check if buffer is empty
bool full() const;            // Check if buffer is full
std::size_t size() const;     // Get current number of elements
std::size_t capacity() const; // Get maximum capacity
void clear();                 // Remove all elements
```

### Building

The library uses Meson build system:

```bash
meson setup build
cd build
ninja
```

### Testing

Run the test executable:

```bash
./ringBufferTest
```

The test demonstrates:
- Basic push/pop operations
- Overflow handling
- Move semantics (copy operations are disabled)
- Edge cases
- Wrap-around behavior

## Span

A minimal C++17 implementation of `std::span` as a header-only library.

### Features

- **C++17 Compatible**: Works with C++17 standard
- **Header-Only**: No separate implementation file needed
- **Generic**: Works with any type
- **STL Compatible**: Supports standard algorithms and iterators
- **Exception Safe**: Proper exception handling for bounds checking
- **Move Semantics**: Supports move construction and assignment (copy operations are disabled)
- **constexpr**: Most operations can be evaluated at compile time

### Usage

```cpp
#include "Span.h"
#include <iostream>
#include <vector>

using namespace utils;

int main() {
    // Create span from array
    int arr[] = {1, 2, 3, 4, 5};
    Span<int> span(arr);

    // Access elements
    std::cout << "Size: " << span.size() << std::endl;
    std::cout << "First: " << span.front() << std::endl;
    std::cout << "Last: " << span.back() << std::endl;

    // Iterate
    for (const auto& element : span) {
        std::cout << element << " ";
    }
    std::cout << std::endl;

    // Create subspan
    auto sub = span.subspan(1, 3);
    for (const auto& element : sub) {
        std::cout << element << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

### API Reference

#### Constructors
```cpp
Span() noexcept;                                    // Default constructor
Span(T* ptr, size_type count) noexcept;            // From pointer and size
Span(T (&arr)[N]) noexcept;                        // From array
Span(std::array<T, N>& arr) noexcept;              // From std::array
Span(const std::array<T, N>& arr) noexcept;        // From const std::array
Span(std::vector<T>& vec) noexcept;                // From std::vector
Span(const std::vector<T>& vec) noexcept;          // From const std::vector
```

#### Element Access
```cpp
T& operator[](size_type idx) const;                // Access element at index
T& front() const;                                  // Access first element
T& back() const;                                   // Access last element
T* data() const noexcept;                          // Get pointer to data
```

#### Capacity
```cpp
size_type size() const noexcept;                   // Get number of elements
size_type size_bytes() const noexcept;             // Get size in bytes
bool empty() const noexcept;                       // Check if empty
```

#### Iterators
```cpp
iterator begin() const noexcept;                   // Iterator to beginning
iterator end() const noexcept;                     // Iterator to end
const_iterator cbegin() const noexcept;            // Const iterator to beginning
const_iterator cend() const noexcept;              // Const iterator to end
reverse_iterator rbegin() const noexcept;          // Reverse iterator to beginning
reverse_iterator rend() const noexcept;            // Reverse iterator to end
const_reverse_iterator crbegin() const noexcept;   // Const reverse iterator to beginning
const_reverse_iterator crend() const noexcept;     // Const reverse iterator to end
```

#### Operations
```cpp
Span<T> subspan(size_type offset, size_type count = dynamic_extent) const;
```

### Testing

Run the test executable:

```bash
./spanTest
```

The test demonstrates:
- Basic span operations
- Iteration (forward, reverse, range-based)
- Construction from different containers
- Subspan operations
- Empty span handling
- STL algorithm compatibility
- Error handling and bounds checking

## SlidingWindow

A generic sliding window buffer that maintains a fixed number of frames, automatically dropping the oldest frame when the window is full. Uses template parameters for frame size (like `std::array`) for compile-time optimization and type safety.

### Features

- **Generic**: Works with any data type
- **Template-based**: Frame size as template parameter (like `std::array`)
- **Compile-time Safety**: Frame size mismatches caught at compile time
- **Frame-based**: Configurable frame size for different use cases
- **Sliding behavior**: Automatically drops oldest frame when full
- **Statistics tracking**: Monitors added/dropped frames and drop rate
- **Header-Only**: No separate implementation file needed
- **STL Compatible**: Full iterator support for algorithms
- **constexpr**: Most operations can be evaluated at compile time

### Usage

```cpp
#include "SlidingWindow.h"
#include <iostream>
#include <vector>

using namespace utils;

int main() {
    // Create a window with 3 frames of 4 items each
    SlidingWindow<int, 4> window(3);

    // Add frames
    std::vector<int> frame1 = {1, 2, 3, 4};
    std::vector<int> frame2 = {5, 6, 7, 8};
    std::vector<int> frame3 = {9, 10, 11, 12};

    window.addFrame(frame1);
    window.addFrame(frame2);
    window.addFrame(frame3);

    // Add another frame - drops the oldest
    std::vector<int> frame4 = {13, 14, 15, 16};
    window.addFrame(frame4);

    // Access frames
    std::cout << "Frame count: " << window.getFrameCount() << std::endl;
    std::cout << "Dropped frames: " << window.getTotalFramesDropped() << std::endl;

    // Iterate over frames
    for (const auto& frame : window) {
        for (int item : frame) {
            std::cout << item << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
```

### API Reference

#### Constructor
```cpp
SlidingWindow(size_type windowSize);
```
Creates a sliding window with specified maximum frames. Frame size is specified as a template parameter.

#### Frame Operations
```cpp
void addFrame(const std::vector<T>& frame);        // Add frame from vector
void addFrame(const T* data, size_type count);     // Add frame from raw data
void addFrame(const T& item);                      // Add single item (frameSize=1)
template<std::size_t N>
void addFrame(const std::array<T, N>& frame);      // Add frame from std::array (compile-time check)
```

#### Access
```cpp
const std::vector<Frame>& getFrames() const;       // Get all frames
const Frame& getFrame(size_type index) const;      // Get specific frame
const Frame& getLatestFrame() const;               // Get newest frame
```

#### State Information
```cpp
size_type getFrameCount() const;                   // Current frame count
size_type getWindowSize() const;                   // Maximum frames
static constexpr size_type getFrameSize() noexcept; // Items per frame (compile-time)
static constexpr size_type frame_size;             // Frame size constant
bool isFull() const;                               // Check if full
bool isEmpty() const;                              // Check if empty
```

#### Statistics
```cpp
size_type getTotalItems() const;                   // Total items across frames
size_type getTotalFramesAdded() const;             // Total frames added
size_type getTotalFramesDropped() const;           // Total frames dropped
double getDropRate() const;                        // Drop rate percentage
```

#### Utility
```cpp
void clear();                                      // Clear all frames
void reset();                                      // Reset window and statistics
```

#### Iterators
```cpp
auto begin() const;                                // Iterator to beginning
auto end() const;                                  // Iterator to end
auto rbegin() const;                               // Reverse iterator to beginning
auto rend() const;                                 // Reverse iterator to end
```

### Testing

Run the test executable:

```bash
./slidingWindowTest
```

The test demonstrates:
- Basic sliding window operations
- Single item frames
- Audio preroll simulation
- Statistics and drop rate calculation
- Error handling and validation
- Forward and reverse iteration
- std::array support with compile-time size checking
- Compile-time frame size features


### Implementation Notes

The SlidingWindow uses a flat `std::vector<T>` container internally for efficient memory layout. Frame boundaries are handled internally, allowing for:

- **Efficient iteration**: Direct iteration over all items
- **Zero-copy frame views**: Use `getFrameView()` to access frames without copying
- **Direct data access**: Use `data()` and `size()` for raw access
- **STL algorithm compatibility**: Works seamlessly with standard algorithms

The flat container approach provides better cache locality and reduces memory fragmentation compared to a vector of vectors.

### Template Benefits

Using frame size as a template parameter provides several advantages:

- **Compile-time Optimization**: Frame size known at compile time allows for better optimizations
- **Type Safety**: Prevents runtime frame size mismatches with `static_assert`
- **Performance**: No runtime frame size checks needed
- **Consistency**: Follows the same pattern as `std::array<T, N>`
- **std::array Support**: Direct support for `std::array` with compile-time size checking

Example of compile-time safety:
```cpp
SlidingWindow<int, 4> window(3);  // OK
window.addFrame({1, 2, 3, 4});    // OK
window.addFrame({1, 2, 3});       // Runtime error: size mismatch

std::array<int, 4> frame = {1, 2, 3, 4};
window.addFrame(frame);           // OK

std::array<int, 3> wrongFrame = {1, 2, 3};
window.addFrame(wrongFrame);      // Compile-time error: static_assert failure
```
