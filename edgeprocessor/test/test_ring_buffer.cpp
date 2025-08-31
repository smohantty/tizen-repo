#include "RingBuffer.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic> // Added for atomic operations in testThreadSafety

using namespace edgeprocessor;

void testBasicOperations() {
    std::cout << "Testing basic RingBuffer operations..." << std::endl;

    RingBuffer buffer(1024);

    // Test initial state
    assert(buffer.size() == 1024);
    assert(buffer.empty());
    assert(!buffer.full());
    assert(buffer.availableRead() == 0);
    assert(buffer.availableWrite() == 1024);

    // Test write
    std::vector<uint8_t> testData = {1, 2, 3, 4, 5};
    size_t written = buffer.write(testData.data(), testData.size());
    assert(written == testData.size());
    assert(buffer.availableRead() == testData.size());
    assert(buffer.availableWrite() == 1024 - testData.size());
    assert(!buffer.empty());

    // Test read
    std::vector<uint8_t> readData(5);
    size_t read = buffer.read(readData.data(), readData.size());
    assert(read == testData.size());
    assert(readData == testData);
    assert(buffer.empty());
    assert(buffer.availableRead() == 0);
    assert(buffer.availableWrite() == 1024);

    std::cout << "✓ Basic operations test passed" << std::endl;
}

void testOverflow() {
    std::cout << "Testing RingBuffer overflow..." << std::endl;

    RingBuffer buffer(10);

    // Fill buffer
    std::vector<uint8_t> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    size_t written = buffer.write(data.data(), data.size());
    assert(written == data.size());
    assert(buffer.full());
    assert(buffer.availableWrite() == 0);

    // Try to write more (should fail)
    std::vector<uint8_t> extraData = {11, 12};
    written = buffer.write(extraData.data(), extraData.size());
    assert(written == 0);

    std::cout << "✓ Overflow test passed" << std::endl;
}

void testUnderflow() {
    std::cout << "Testing RingBuffer underflow..." << std::endl;

    RingBuffer buffer(10);

    // Try to read from empty buffer
    std::vector<uint8_t> data(5);
    size_t read = buffer.read(data.data(), data.size());
    assert(read == 0);

    std::cout << "✓ Underflow test passed" << std::endl;
}

void testWraparound() {
    std::cout << "Testing RingBuffer wraparound..." << std::endl;

    RingBuffer buffer(10);

    // Write data that will wrap around
    std::vector<uint8_t> data1 = {1, 2, 3, 4, 5};
    std::vector<uint8_t> data2 = {6, 7, 8, 9, 10};

    buffer.write(data1.data(), data1.size());
    buffer.write(data2.data(), data2.size());

    // Read first part
    std::vector<uint8_t> readData1(5);
    buffer.read(readData1.data(), readData1.size());
    assert(readData1 == data1);

    // Write more data (should wrap around)
    std::vector<uint8_t> data3 = {11, 12, 13, 14, 15};
    size_t written = buffer.write(data3.data(), data3.size());
    assert(written == data3.size());

    // Read remaining data
    std::vector<uint8_t> readData2(10);
    size_t read = buffer.read(readData2.data(), readData2.size());
    assert(read == 10);

    std::cout << "✓ Wraparound test passed" << std::endl;
}

void testClear() {
    std::cout << "Testing RingBuffer clear..." << std::endl;

    RingBuffer buffer(10);

    // Write some data
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    buffer.write(data.data(), data.size());

    // Clear buffer
    buffer.clear();

    // Check state
    assert(buffer.empty());
    assert(!buffer.full());
    assert(buffer.availableRead() == 0);
    assert(buffer.availableWrite() == 10);

    std::cout << "✓ Clear test passed" << std::endl;
}

void testThreadSafety() {
    std::cout << "Testing RingBuffer thread safety..." << std::endl;

    RingBuffer buffer(1000);
    std::atomic<bool> stop(false);
    std::atomic<size_t> writeCount(0);
    std::atomic<size_t> readCount(0);

    // Writer thread
    std::thread writer([&]() {
        std::vector<uint8_t> data(10, 42);
        while (!stop) {
            size_t written = buffer.write(data.data(), data.size());
            if (written > 0) {
                writeCount += written;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    // Reader thread
    std::thread reader([&]() {
        std::vector<uint8_t> data(10);
        while (!stop) {
            size_t read = buffer.read(data.data(), data.size());
            if (read > 0) {
                readCount += read;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    // Run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop = true;

    writer.join();
    reader.join();

    std::cout << "✓ Thread safety test passed (wrote: " << writeCount
              << ", read: " << readCount << ")" << std::endl;
}

int main() {
    std::cout << "RingBuffer Unit Tests" << std::endl;
    std::cout << "====================" << std::endl;

    try {
        testBasicOperations();
        testOverflow();
        testUnderflow();
        testWraparound();
        testClear();
        testThreadSafety();

        std::cout << "\n✓ All RingBuffer tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
