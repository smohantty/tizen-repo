#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <functional>

using namespace std::chrono;

namespace vtd {

struct TouchPoint {
    steady_clock::time_point ts;
    float x = 0.0f;
    float y = 0.0f;
    bool touching = false;
};

// --------------------- Device Types ---------------------
enum class DeviceType {
    Linux,          // Linux uinput device (requires /dev/uinput access)
    Mock            // Mock device for testing (no actual output)
};

// --------------------- Config ---------------------
struct Config {
    int screenWidth = 1920;
    int screenHeight = 1080;
    double inputRateHz = 30.0;
    double outputRateHz = 120.0;
    double maxInputHistorySec = 1.0;
    double maxExtrapolationMs = 50.0;  // Maximum extrapolation time in milliseconds
    double touchTimeoutMs = 200.0;     // Auto-release touch after this timeout (0 = disabled)
    std::string deviceName = "Virtual IR Touch";

    // Device configuration
    DeviceType deviceType = DeviceType::Mock; // Device type selection

    // Recording configuration - works with all backends
    bool enableRawInputRecording = false;        // Record raw input from user
    bool enableUpsampledRecording = false;       // Record upsampled touchpoints
    std::string rawInputRecordPath = "./raw_input.json";         // File path for raw input recording
    std::string upsampledRecordPath = "./upsampled_output.json"; // File path for upsampled recording

    // Default configuration factory
    static Config getDefault();
};

class VirtualTouchDevice {
public:
    explicit VirtualTouchDevice(const Config& cfg);
    ~VirtualTouchDevice();

    bool start();
    void stop();
    void pushInputPoint(const TouchPoint& p);

    // Event callback interface (works with all device types)
    void setEventCallback(std::function<void(const TouchPoint&)> callback);

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace vtd
