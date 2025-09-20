// Test the recording functionality with Mock device
#include "VirtualTouchDevice.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <cassert>

using namespace vtd;

int main() {
    std::cout << "🎬 Testing Recording Functionality..." << std::endl;

    // Configure device with recording enabled
    Config cfg;
    cfg.deviceType = DeviceType::Mock; // Use Mock device with recording
    cfg.enableRawInputRecording = true;
    cfg.enableUpsampledRecording = true;
    cfg.rawInputRecordPath = "./test_raw_recording.json";
    cfg.upsampledRecordPath = "./test_upsampled_recording.json";
    cfg.screenWidth = 1920;
    cfg.screenHeight = 1080;
    cfg.deviceName = "Recording Test Device";
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

    std::cout << "⏹️  Stopping device and saving recordings..." << std::endl;
    device.stop();

    // Give a moment for files to be written
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return 0;
}