// Test the Record device functionality
#include "VirtualTouchDevice.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace vtd;

int main() {
    std::cout << "Testing Record Device..." << std::endl;

    // Configure device to record events
    Config cfg;
    cfg.deviceType = DeviceType::Record;
    cfg.recordFilePath = "test_recording.json";
    cfg.screenWidth = 1920;
    cfg.screenHeight = 1080;

    VirtualTouchDevice device(cfg);

    if (!device.start()) {
        std::cerr << "Failed to start device" << std::endl;
        return 1;
    }

    // Simulate some touch events
    std::cout << "Generating test touch events..." << std::endl;

    auto now = std::chrono::steady_clock::now();

    // Touch down
    TouchPoint point1;
    point1.ts = now;
    point1.x = 100.0f;
    point1.y = 200.0f;
    point1.touching = true;
    device.pushInputPoint(point1);

    // Move
    TouchPoint point2;
    point2.ts = now + std::chrono::milliseconds(50);
    point2.x = 150.0f;
    point2.y = 250.0f;
    point2.touching = true;
    device.pushInputPoint(point2);

    // Touch up
    TouchPoint point3;
    point3.ts = now + std::chrono::milliseconds(100);
    point3.x = 200.0f;
    point3.y = 300.0f;
    point3.touching = false;
    device.pushInputPoint(point3);

    // Let the device process the events
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::cout << "Stopping device and saving recording..." << std::endl;
    device.stop();

    std::cout << "Test completed. Check test_recording.json for results." << std::endl;
    return 0;
}
