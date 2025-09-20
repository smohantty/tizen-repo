// Test the recording functionality with Mock device
#include "VirtualTouchDevice.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <cassert>
#include <vector>

using namespace vtd;


class InputDevice
{
private:
std::unique_ptr<vtd::VirtualTouchDevice> mDevice;
public:
InputDevice()
{
    Config cfg;
    cfg.deviceType = DeviceType::Mock;
    cfg.enableRawInputRecording = true;
    cfg.enableUpsampledRecording = true;
    cfg.rawInputRecordPath = "./dump/raw_recording.json";
    cfg.upsampledRecordPath = "./dump/upsampled_recording.json";
    cfg.screenWidth = 1920;
    cfg.screenHeight = 1080;
    cfg.deviceName = "IR Device";
    cfg.smoothingType = SmoothingType::EMA;
    cfg.maxExtrapolationMs = 50.0;

    mDevice = std::make_unique<vtd::VirtualTouchDevice>(cfg);

    mDevice->start();
}

void playGesture(std::vector<TouchPoint>& points){
    for (size_t i = 0; i < points.size(); ++i) {
        mDevice->pushInputPoint(points[i]);
        std::this_thread::sleep_for(std::chrono::milliseconds(34));
    }
}

};

void test_swipe_1(InputDevice& device)
{
    auto now = std::chrono::steady_clock::now();
    std::vector<TouchPoint> inputPoints = {
        {now, 100.0f, 200.0f, true},                    // Touch down
        {now + std::chrono::milliseconds(33), 120.0f, 202.0f, true},  // Small movement
        {now + std::chrono::milliseconds(67), 140.0f, 204.0f, true},  // Continue movement
        {now + std::chrono::milliseconds(100), 160.0f, 205.0f, true}, // Final position
        {now + std::chrono::milliseconds(133), 180.0f, 205.0f, false} // Touch release
    };
    device.playGesture(inputPoints);
}

void test_pan_change_direction(InputDevice& device)
{
    auto now = std::chrono::steady_clock::now();
    std::vector<TouchPoint> inputPoints = {
        {now, 100.0f, 200.0f, true},                    // Touch down
        {now + std::chrono::milliseconds(33), 120.0f, 202.0f, true},  // Move right
        {now + std::chrono::milliseconds(67), 140.0f, 204.0f, true},  // Continue right
        {now + std::chrono::milliseconds(100), 160.0f, 205.0f, true}, // Continue right
        {now + std::chrono::milliseconds(133), 180.0f, 205.0f, true}, // Continue right
        {now + std::chrono::milliseconds(167), 200.0f, 205.0f, true}, // Peak right position
        {now + std::chrono::milliseconds(200), 180.0f, 205.0f, true}, // Start moving left
        {now + std::chrono::milliseconds(233), 160.0f, 205.0f, true}, // Continue left
        {now + std::chrono::milliseconds(267), 140.0f, 205.0f, true}, // Continue left
        {now + std::chrono::milliseconds(300), 120.0f, 205.0f, true}, // Continue left
        {now + std::chrono::milliseconds(333), 100.0f, 205.0f, true}, // Continue left
        {now + std::chrono::milliseconds(367), 80.0f, 205.0f, true},  // Peak left position
        {now + std::chrono::milliseconds(400), 100.0f, 205.0f, true}, // Start moving right again
        {now + std::chrono::milliseconds(433), 120.0f, 205.0f, true}, // Continue right
        {now + std::chrono::milliseconds(467), 140.0f, 205.0f, true}, // Continue right

        {now + std::chrono::milliseconds(500), 150.0f, 205.0f, false} // Touch release
    };
    device.playGesture(inputPoints);
}

void test_swipe_missing_release(InputDevice& device)
{
    auto now = std::chrono::steady_clock::now();
    std::vector<TouchPoint> inputPoints = {
        {now, 100.0f, 200.0f, true},                    // Touch down
        {now + std::chrono::milliseconds(33), 120.0f, 202.0f, true},  // Small movement
        {now + std::chrono::milliseconds(67), 140.0f, 204.0f, true},  // Continue movement
        {now + std::chrono::milliseconds(100), 160.0f, 205.0f, true}, // Final position
    };
    device.playGesture(inputPoints);
}

int main() {
    InputDevice device;
    //test_swipe_1(device);
    //test_swipe_missing_release(device);
    test_pan_change_direction(device);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return 0;
}