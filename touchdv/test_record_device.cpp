// Test the recording functionality with Mock device
#include "VirtualTouchDevice.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <cassert>
#include <vector>

using namespace vtd;

int main() {
    std::cout << "🎬 Testing Recording Functionality..." << std::endl;

    // Configure device with recording enabled
    Config cfg;
    cfg.deviceType = DeviceType::Mock; // Use Mock device with recording
    cfg.enableRawInputRecording = true;
    cfg.enableUpsampledRecording = true;
    cfg.rawInputRecordPath = "./dump/raw_recording.json";
    cfg.upsampledRecordPath = "./dump/upsampled_recording.json";
    cfg.screenWidth = 1920;
    cfg.screenHeight = 1080;
    cfg.deviceName = "Recording Test Device";
    cfg.smoothingType = SmoothingType::EMA; // Use raw data for testing
    cfg.maxExtrapolationMs = 50.0; // Keep default extrapolation limit

    VirtualTouchDevice device(cfg);

    if (!device.start()) {
        std::cerr << "❌ Failed to start device" << std::endl;
        return 1;
    }

    std::cout << "📝 Generating test touch events at 30fps intervals..." << std::endl;

    auto now = std::chrono::steady_clock::now();

    // Simulate realistic touch input at 30fps (33.33ms intervals)
    std::vector<TouchPoint> inputPoints = {
        {now, 100.0f, 200.0f, true},                    // Touch down
        {now + std::chrono::milliseconds(33), 120.0f, 202.0f, true},  // Small movement
        {now + std::chrono::milliseconds(67), 140.0f, 204.0f, true},  // Continue movement
        {now + std::chrono::milliseconds(100), 160.0f, 205.0f, true}, // Final position
        {now + std::chrono::milliseconds(133), 180.0f, 205.0f, false} // Touch release
    };

    // Send events at realistic 30fps timing
    for (size_t i = 0; i < inputPoints.size(); ++i) {
        const auto& point = inputPoints[i];
        device.pushInputPoint(point);

        std::cout << "  Sent point " << (i+1) << " at "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                     point.ts.time_since_epoch()).count() % 10000 << "ms"
                  << " (" << point.x << ", " << point.y << ", "
                  << (point.touching ? "touch" : "release") << ")" << std::endl;

        // Wait for 30fps interval (33.33ms) before sending next point
        if (i < inputPoints.size() - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(34)); // 34ms to account for processing
        }
    }

    // Let the device process the events for a while
    std::cout << "⏳ Processing events..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::cout << "⏹️  Stopping device and saving recordings..." << std::endl;
    device.stop();

    // Give a moment for files to be written
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return 0;
}