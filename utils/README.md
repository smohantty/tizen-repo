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
