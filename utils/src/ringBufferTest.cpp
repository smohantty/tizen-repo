#include "RingBuffer.h"
#include "RingBuffer.cpp"  // Include implementation for template instantiation
#include <iostream>
#include <string>
#include <cassert>

using namespace utils;

void testIntRingBuffer() {
    std::cout << "=== Testing Int RingBuffer ===" << std::endl;

    // Test basic functionality
    RingBuffer<int> buffer(5);

    std::cout << "Initial state - Empty: " << buffer.empty()
              << ", Full: " << buffer.full()
              << ", Size: " << buffer.size()
              << ", Capacity: " << buffer.capacity() << std::endl;

    // Test pushing elements
    std::cout << "\nPushing elements..." << std::endl;
    for (int i = 1; i <= 3; ++i) {
        bool success = buffer.push(i);
        std::cout << "Pushed " << i << ": " << (success ? "SUCCESS" : "FAILED") << std::endl;
    }

    std::cout << "After pushing - Empty: " << buffer.empty()
              << ", Full: " << buffer.full()
              << ", Size: " << buffer.size() << std::endl;

    // Test front operation
    int frontValue;
    if (buffer.front(frontValue)) {
        std::cout << "Front element: " << frontValue << std::endl;
    }

    // Test popping elements
    std::cout << "\nPopping elements..." << std::endl;
    for (int i = 0; i < 3; ++i) {
        int value;
        bool success = buffer.pop(value);
        std::cout << "Popped: " << (success ? std::to_string(value) : "FAILED") << std::endl;
    }

    std::cout << "After popping - Empty: " << buffer.empty()
              << ", Size: " << buffer.size() << std::endl;

    // Test overflow behavior
    std::cout << "\nTesting overflow behavior..." << std::endl;
    for (int i = 1; i <= 7; ++i) {
        bool success = buffer.push(i);
        std::cout << "Pushed " << i << ": " << (success ? "SUCCESS" : "FAILED") << std::endl;
    }

    // Test popping from full buffer
    std::cout << "\nPopping from full buffer..." << std::endl;
    for (int i = 0; i < 5; ++i) {
        int value;
        bool success = buffer.pop(value);
        std::cout << "Popped: " << (success ? std::to_string(value) : "FAILED") << std::endl;
    }
}

void testStringRingBuffer() {
    std::cout << "\n=== Testing String RingBuffer ===" << std::endl;

    RingBuffer<std::string> buffer(3);

    // Test move semantics
    std::cout << "Testing move semantics..." << std::endl;
    std::string str1 = "Hello";
    std::string str2 = "World";
    std::string str3 = "C++17";

    buffer.push(std::move(str1));
    buffer.push(std::move(str2));
    buffer.push(std::move(str3));

    std::cout << "After moving strings - Size: " << buffer.size() << std::endl;

    // Test popping strings
    std::cout << "\nPopping strings..." << std::endl;
    for (int i = 0; i < 3; ++i) {
        std::string value;
        bool success = buffer.pop(value);
        std::cout << "Popped: " << (success ? value : "FAILED") << std::endl;
    }
}

void testMoveSemantics() {
    std::cout << "\n=== Testing Move Semantics ===" << std::endl;

    // Create original buffer
    RingBuffer<int> original(3);
    original.push(1);
    original.push(2);
    original.push(3);

    std::cout << "Original buffer size: " << original.size() << std::endl;

    // Test move constructor
    RingBuffer<int> moved(std::move(original));
    std::cout << "Moved buffer size: " << moved.size() << std::endl;
    // Note: original is now in a moved-from state, accessing it is undefined behavior
    // std::cout << "Original buffer size after move: " << original.size() << std::endl;

    // Test move assignment with a fresh buffer
    RingBuffer<int> moveAssigned(1);
    RingBuffer<int> tempBuffer(2);
    tempBuffer.push(10);
    tempBuffer.push(20);
    moveAssigned = std::move(tempBuffer);
    std::cout << "Move assigned buffer size: " << moveAssigned.size() << std::endl;
}

void testEdgeCases() {
    std::cout << "\n=== Testing Edge Cases ===" << std::endl;

    // Test empty buffer operations
    RingBuffer<int> buffer(2);

    int value;
    bool success = buffer.pop(value);
    std::cout << "Pop from empty buffer: " << (success ? "SUCCESS" : "FAILED") << std::endl;

    success = buffer.front(value);
    std::cout << "Front from empty buffer: " << (success ? "SUCCESS" : "FAILED") << std::endl;

    // Test clear operation
    buffer.push(1);
    buffer.push(2);
    std::cout << "Before clear - Size: " << buffer.size() << std::endl;

    buffer.clear();
    std::cout << "After clear - Size: " << buffer.size() << ", Empty: " << buffer.empty() << std::endl;

    // Test wrap-around behavior
    std::cout << "\nTesting wrap-around behavior..." << std::endl;
    RingBuffer<int> wrapBuffer(3);

    // Fill the buffer
    wrapBuffer.push(1);
    wrapBuffer.push(2);
    wrapBuffer.push(3);

    // Pop one element to create space
    int temp;
    wrapBuffer.pop(temp);
    std::cout << "Popped: " << temp << std::endl;

    // Push new element (should wrap around)
    bool pushSuccess = wrapBuffer.push(4);
    std::cout << "Pushed 4 after wrap-around: " << (pushSuccess ? "SUCCESS" : "FAILED") << std::endl;

    // Pop all elements to see the order
    std::cout << "Elements in wrap-around order: ";
    while (wrapBuffer.pop(temp)) {
        std::cout << temp << " ";
    }
    std::cout << std::endl;
}

int main() {
    try {
        testIntRingBuffer();
        testStringRingBuffer();
        testMoveSemantics();
        testEdgeCases();

        std::cout << "\n=== All tests completed successfully! ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
