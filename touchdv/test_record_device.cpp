// Test the Record device functionality
#include "VirtualTouchDevice.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <cassert>

using namespace vtd;

int main() {
    std::cout << "🎬 Testing Record Device Functionality..." << std::endl;

    // Configure device to record events
    Config cfg;
    cfg.deviceType = DeviceType::Record;
    cfg.recordFilePath = "./test_recording.json";
    cfg.screenWidth = 1920;
    cfg.screenHeight = 1080;
    cfg.deviceName = "Record Test Device";
    cfg.smoothingType = SmoothingType::None; // Use raw data for testing

    VirtualTouchDevice device(cfg);

    if (!device.start()) {
        std::cerr << "❌ Failed to start device" << std::endl;
        return 1;
    }

    std::cout << "📝 Generating test touch events..." << std::endl;

    auto now = std::chrono::steady_clock::now();

    // Simulate a tap gesture: down -> move -> up
    TouchPoint point1;
    point1.ts = now;
    point1.x = 100.0f;
    point1.y = 200.0f;
    point1.touching = true;
    device.pushInputPoint(point1);

    // Small movement during touch
    TouchPoint point2;
    point2.ts = now + std::chrono::milliseconds(50);
    point2.x = 105.0f;
    point2.y = 205.0f;
    point2.touching = true;
    device.pushInputPoint(point2);

    // Touch release
    TouchPoint point3;
    point3.ts = now + std::chrono::milliseconds(100);
    point3.x = 105.0f;
    point3.y = 205.0f;
    point3.touching = false;
    device.pushInputPoint(point3);

    // Let the device process the events
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    std::cout << "⏹️  Stopping device and saving recording..." << std::endl;
    device.stop();

    // Verify the recording file was created
    std::ifstream file(cfg.recordFilePath);
    if (!file.is_open()) {
        std::cerr << "❌ Failed to create recording file: " << cfg.recordFilePath << std::endl;
        return 1;
    }

    // Read and validate the file content
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Basic validation checks
    bool hasDeviceName = content.find("\"deviceName\": \"Record Test Device\"") != std::string::npos;
    bool hasScreenWidth = content.find("\"screenWidth\": 1920") != std::string::npos;
    bool hasScreenHeight = content.find("\"screenHeight\": 1080") != std::string::npos;
    bool hasEvents = content.find("\"events\":") != std::string::npos;
    bool hasTotalEvents = content.find("\"totalEvents\":") != std::string::npos;

    if (!hasDeviceName || !hasScreenWidth || !hasScreenHeight || !hasEvents || !hasTotalEvents) {
        std::cerr << "❌ Recording file validation failed" << std::endl;
        std::cerr << "Content preview: " << content.substr(0, 200) << "..." << std::endl;
        return 1;
    }

    std::cout << "✅ Record device test passed!" << std::endl;
    std::cout << "📁 Recording saved to: " << cfg.recordFilePath << std::endl;
    std::cout << "🔍 File size: " << content.length() << " bytes" << std::endl;

    // Clean up test file (disabled for inspection)
    // std::remove(cfg.recordFilePath.c_str());
    std::cout << "📁 Test file preserved for inspection: " << cfg.recordFilePath << std::endl;

    return 0;
}