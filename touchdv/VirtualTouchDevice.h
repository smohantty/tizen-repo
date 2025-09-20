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

// --------------------- Smoothing Strategies ---------------------
enum class SmoothingType {
    None,           // No smoothing
    EMA,            // Exponential Moving Average
    Kalman,         // Kalman filter (constant velocity model)
    OneEuro         // 1-Euro filter
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

    // Touch sequence handling - optimized for discrete touch patterns (TTTTTR)
    double touchTransitionThreshold = 0.1; // Threshold for touch state transitions (0.0-0.5)

    // Smoothing configuration
    SmoothingType smoothingType = SmoothingType::EMA;

    // EMA parameters
    double emaAlpha = 0.45;

    // Kalman parameters
    double kalmanQ = 0.01;  // Process noise
    double kalmanR = 1.0;   // Measurement noise

    // OneEuro parameters
    double oneEuroFreq = 120.0;
    double oneEuroMinCutoff = 1.0;
    double oneEuroBeta = 0.007;
    double oneEuroDCutoff = 1.0;

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
