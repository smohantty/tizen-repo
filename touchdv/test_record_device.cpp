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

    // Verify both recording files were created
    std::ifstream rawFile(cfg.rawInputRecordPath);
    std::ifstream upsampledFile(cfg.upsampledRecordPath);

    if (!rawFile.is_open()) {
        std::cerr << "❌ Failed to create raw input recording file: " << cfg.rawInputRecordPath << std::endl;
        return 1;
    }

    if (!upsampledFile.is_open()) {
        std::cerr << "❌ Failed to create upsampled recording file: " << cfg.upsampledRecordPath << std::endl;
        return 1;
    }

    // Read and validate the raw input file content
    std::string rawContent((std::istreambuf_iterator<char>(rawFile)), std::istreambuf_iterator<char>());
    rawFile.close();

    // Read and validate the upsampled file content
    std::string upsampledContent((std::istreambuf_iterator<char>(upsampledFile)), std::istreambuf_iterator<char>());
    upsampledFile.close();

    // Basic validation checks for raw input file
    bool rawHasDeviceName = rawContent.find("\"deviceName\": \"Recording Test Device\"") != std::string::npos;
    bool rawHasScreenWidth = rawContent.find("\"screenWidth\": 1920") != std::string::npos;
    bool rawHasRecordType = rawContent.find("\"recordType\": \"raw_input\"") != std::string::npos;
    bool rawHasEvents = rawContent.find("\"events\":") != std::string::npos;

    // Basic validation checks for upsampled file
    bool upsampledHasDeviceName = upsampledContent.find("\"deviceName\": \"Recording Test Device\"") != std::string::npos;
    bool upsampledHasScreenWidth = upsampledContent.find("\"screenWidth\": 1920") != std::string::npos;
    bool upsampledHasRecordType = upsampledContent.find("\"recordType\": \"upsampled_output\"") != std::string::npos;
    bool upsampledHasEvents = upsampledContent.find("\"events\":") != std::string::npos;

    if (!rawHasDeviceName || !rawHasScreenWidth || !rawHasRecordType || !rawHasEvents) {
        std::cerr << "❌ Raw input recording file validation failed" << std::endl;
        std::cerr << "Content preview: " << rawContent.substr(0, 200) << "..." << std::endl;
        return 1;
    }

    if (!upsampledHasDeviceName || !upsampledHasScreenWidth || !upsampledHasRecordType || !upsampledHasEvents) {
        std::cerr << "❌ Upsampled recording file validation failed" << std::endl;
        std::cerr << "Content preview: " << upsampledContent.substr(0, 200) << "..." << std::endl;
        return 1;
    }

    std::cout << "✅ Recording functionality test passed!" << std::endl;
    std::cout << "📁 Raw input recording saved to: " << cfg.rawInputRecordPath << std::endl;
    std::cout << "📁 Upsampled recording saved to: " << cfg.upsampledRecordPath << std::endl;
    std::cout << "🔍 Raw file size: " << rawContent.length() << " bytes" << std::endl;
    std::cout << "🔍 Upsampled file size: " << upsampledContent.length() << " bytes" << std::endl;

    // Clean up test files (disabled for inspection)
    // std::remove(cfg.rawInputRecordPath.c_str());
    // std::remove(cfg.upsampledRecordPath.c_str());
    std::cout << "📁 Test files preserved for inspection" << std::endl;

    return 0;
}