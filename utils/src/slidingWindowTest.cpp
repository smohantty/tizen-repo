#include "SlidingWindow.h"
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <iomanip>

using namespace utils;

void testBasicSlidingWindow() {
    std::cout << "=== Testing Basic SlidingWindow ===" << std::endl;

    // Create a window with 3 frames of 4 items each
    SlidingWindow<int, 4> window(3);

    std::cout << "Window size: " << window.getWindowSize() << std::endl;
    std::cout << "Frame size: " << window.getFrameSize() << std::endl;
    std::cout << "Initial frame count: " << window.getFrameCount() << std::endl;
    std::cout << "Empty: " << window.isEmpty() << std::endl;
    std::cout << "Full: " << window.isFull() << std::endl;

    // Add frames
    std::vector<int> frame1 = {1, 2, 3, 4};
    std::vector<int> frame2 = {5, 6, 7, 8};
    std::vector<int> frame3 = {9, 10, 11, 12};

    window.addFrame(frame1);
    std::cout << "After frame 1 - Count: " << window.getFrameCount() << std::endl;

    window.addFrame(frame2);
    std::cout << "After frame 2 - Count: " << window.getFrameCount() << std::endl;

    window.addFrame(frame3);
    std::cout << "After frame 3 - Count: " << window.getFrameCount() << std::endl;
    std::cout << "Full: " << window.isFull() << std::endl;

    // Add another frame - should drop the oldest
    std::vector<int> frame4 = {13, 14, 15, 16};
    window.addFrame(frame4);
    std::cout << "After frame 4 - Count: " << window.getFrameCount() << std::endl;

    // Access frames
    std::cout << "Oldest frame: ";
    for (int item : window.getFrame(0)) {
        std::cout << item << " ";
    }
    std::cout << std::endl;

    std::cout << "Newest frame: ";
    for (int item : window.getLatestFrame()) {
        std::cout << item << " ";
    }
    std::cout << std::endl;

    // Test getData() - show all data
    std::cout << "All data: ";
    for (int item : window.getData()) {
        std::cout << item << " ";
    }
    std::cout << std::endl;
}

void testSingleItemFrames() {
    std::cout << "\n=== Testing Single Item Frames ===" << std::endl;

    // Create a window with 5 frames of 1 item each
    SlidingWindow<std::string, 1> window(5);

    // Add single items
    window.addFrame("first");
    window.addFrame("second");
    window.addFrame("third");
    window.addFrame("fourth");
    window.addFrame("fifth");

    std::cout << "Frame count: " << window.getFrameCount() << std::endl;

    // Add more - should drop oldest
    window.addFrame("sixth");
    window.addFrame("seventh");

    std::cout << "After adding more - Count: " << window.getFrameCount() << std::endl;

    // Show all data (flat iteration)
    std::cout << "All data: ";
    for (const auto& item : window) {
        std::cout << item << " ";
    }
    std::cout << std::endl;
}

void testAudioPreroll() {
    std::cout << "\n=== Testing Audio Preroll Simulation ===" << std::endl;

    // Simulate audio preroll: 25 frames of 160 samples each (250ms at 16kHz)
    SlidingWindow<short, 160> audioWindow(25);

    std::cout << "Audio window - Max frames: " << audioWindow.getWindowSize()
              << ", Frame size: " << audioWindow.getFrameSize() << std::endl;

    // Generate some audio frames
    for (int i = 0; i < 30; ++i) {
        std::vector<short> audioFrame(160);
        for (int j = 0; j < 160; ++j) {
            audioFrame[j] = static_cast<short>((i * 1000) + j);
        }

        audioWindow.addFrame(audioFrame);

        if (i % 5 == 0) {
            std::cout << "Frame " << i << " - Count: " << audioWindow.getFrameCount() << std::endl;
        }
    }

    std::cout << "Final - Frames: " << audioWindow.getFrameCount()
              << ", Total items: " << audioWindow.getTotalItems() << std::endl;
}

void testStatistics() {
    std::cout << "\n=== Testing Window Operations ===" << std::endl;

    SlidingWindow<double, 3> window(10);

    // Add many frames
    for (int i = 0; i < 50; ++i) {
        std::vector<double> frame = {i * 1.0, i * 2.0, i * 3.0};
        window.addFrame(frame);
    }

    std::cout << "Current frame count: " << window.getFrameCount() << std::endl;
    std::cout << "Total items: " << window.getTotalItems() << std::endl;
    std::cout << "Is full: " << window.isFull() << std::endl;

    // Reset and check
    window.reset();
    std::cout << "After reset - Frame count: " << window.getFrameCount() << std::endl;
}

void testErrorHandling() {
    std::cout << "\n=== Testing Error Handling ===" << std::endl;

    SlidingWindow<int, 3> window(5);

    try {
        // Try to add frame with wrong size
        std::vector<int> wrongFrame = {1, 2}; // Should be 3
        window.addFrame(wrongFrame);
        std::cout << "Error: Should have thrown exception" << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }

    try {
        // Try to access frame when empty
        auto frame = window.getLatestFrame();
        std::cout << "Error: Should have thrown exception" << std::endl;
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }

    try {
        // Try to create window with zero size
        SlidingWindow<int, 3> badWindow(0);
        std::cout << "Error: Should have thrown exception" << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cout << "Caught expected exception: " << e.what() << std::endl;
    }
}

void testIteration() {
    std::cout << "\n=== Testing Iteration ===" << std::endl;

    SlidingWindow<char, 2> window(4);

    // Add some frames
    window.addFrame({'a', 'b'});
    window.addFrame({'c', 'd'});
    window.addFrame({'e', 'f'});
    window.addFrame({'g', 'h'});
    window.addFrame({'i', 'j'}); // Should drop oldest

    std::cout << "Forward iteration (all items): ";
    for (const auto& item : window) {
        std::cout << item << " ";
    }
    std::cout << std::endl;

    std::cout << "Reverse iteration (all items): ";
    for (auto it = window.rbegin(); it != window.rend(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // Test frame-by-frame iteration
    std::cout << "Frame-by-frame iteration: ";
    for (size_t i = 0; i < window.getFrameCount(); ++i) {
        auto frame = window.getFrame(i);
        std::cout << "(";
        for (size_t j = 0; j < frame.size(); ++j) {
            std::cout << frame[j];
            if (j < frame.size() - 1) std::cout << ",";
        }
        std::cout << ") ";
    }
    std::cout << std::endl;
}

void testFrameView() {
    std::cout << "\n=== Testing Frame View (no copy) ===" << std::endl;

    SlidingWindow<int, 4> window(3);

    // Add frames
    window.addFrame({1, 2, 3, 4});
    window.addFrame({5, 6, 7, 8});
    window.addFrame({9, 10, 11, 12});

    std::cout << "Frame views (no copy):" << std::endl;
    for (size_t i = 0; i < window.getFrameCount(); ++i) {
        auto [begin, end] = window.getFrameView(i);
        std::cout << "Frame " << i << ": ";
        for (auto it = begin; it != end; ++it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;
    }

    // Test with algorithms
    auto [begin, end] = window.getFrameView(1);
    auto maxElement = std::max_element(begin, end);
    std::cout << "Max element in frame 1: " << *maxElement << std::endl;
}

void testDirectAccess() {
    std::cout << "\n=== Testing Direct Data Access ===" << std::endl;

    SlidingWindow<int, 3> window(2);
    window.addFrame({1, 2, 3});
    window.addFrame({4, 5, 6});

    std::cout << "Direct data access: ";
    const int* data = window.data();
    for (size_t i = 0; i < window.size(); ++i) {
        std::cout << data[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "Total size: " << window.size() << std::endl;
    std::cout << "Frame count: " << window.getFrameCount() << std::endl;
}

void testStdArraySupport() {
    std::cout << "\n=== Testing std::array Support ===" << std::endl;

    SlidingWindow<int, 3> window(2);

    // Add frames using std::array (compile-time size check)
    std::array<int, 3> frame1 = {1, 2, 3};
    std::array<int, 3> frame2 = {4, 5, 6};

    window.addFrame(frame1);
    window.addFrame(frame2);

    std::cout << "Added frames using std::array" << std::endl;
    std::cout << "Frame count: " << window.getFrameCount() << std::endl;

    // This would cause a compile-time error:
    // std::array<int, 2> wrongFrame = {1, 2};
    // window.addFrame(wrongFrame); // static_assert failure

    std::cout << "All data: ";
    for (int item : window) {
        std::cout << item << " ";
    }
    std::cout << std::endl;
}

void testCompileTimeFeatures() {
    std::cout << "\n=== Testing Compile-time Features ===" << std::endl;

    // Test static constexpr frame_size
    SlidingWindow<double, 5> window(3);
    std::cout << "Frame size (static): " << SlidingWindow<double, 5>::frame_size << std::endl;
    std::cout << "Frame size (method): " << window.getFrameSize() << std::endl;

    // Test that frame size is truly compile-time
    constexpr auto frameSize = SlidingWindow<int, 8>::frame_size;
    std::cout << "Compile-time frame size: " << frameSize << std::endl;

    // Test static_assert for zero frame size
    // This would cause a compile-time error:
    // SlidingWindow<int, 0> badWindow(5); // static_assert failure
}

int main() {
    try {
        testBasicSlidingWindow();
        testSingleItemFrames();
        testAudioPreroll();
        testStatistics();
        testErrorHandling();
        testIteration();
        testFrameView();
        testDirectAccess();
        testStdArraySupport();
        testCompileTimeFeatures();

        std::cout << "\n=== All SlidingWindow tests completed successfully! ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
