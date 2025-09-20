
#include "VirtualTouchDevice.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <vector>
#include <array>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <limits>
#include <sstream>
#include <iomanip>
#include <functional>
#include <fstream>
#include <cassert>

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

using namespace std::chrono;

namespace vtd {

// --------------------- Config Implementation ---------------------
Config Config::getDefault() {
    Config cfg;
    // Default values are already set in the struct definition
    return cfg;
}

// --------------------- Internal Utility Functions ---------------------
static double toSeconds(steady_clock::duration d) {
    return duration_cast<duration<double>>(d).count();
}


// --------------------- File Recorder Utility ---------------------
// Utility class for recording touch events to JSON files
class FileRecorder {
private:
    std::vector<TouchPoint> mRecordedEvents;
    std::string mFilePath;
    bool mRecordRawInput{true};
    Config mConfig;

public:
    FileRecorder(const std::string& filePath, const Config& config, bool recordRawInput = true)
        : mFilePath(filePath), mRecordRawInput(recordRawInput), mConfig(config) {
        mRecordedEvents.reserve(10000); // Reserve space for many events
    }

    void recordEvent(const TouchPoint& point) {
        mRecordedEvents.push_back(point);
    }

    void saveToFile() {
        if (mRecordedEvents.empty()) return;

        std::ofstream file(mFilePath, std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            std::cerr << "Failed to open " << (mRecordRawInput ? "raw_input" : "upsampled_output") << " record file: " << mFilePath << std::endl;
            return;
        }

        file << "{\n";
        file << "  \"deviceName\": \"" << mConfig.deviceName << "\",\n";
        file << "  \"screenWidth\": " << mConfig.screenWidth << ",\n";
        file << "  \"screenHeight\": " << mConfig.screenHeight << ",\n";
        if (mRecordRawInput) {
            file << "  \"recordType\": \"raw_ir_input\",\n";
            file << "  \"inputRateHz\": " << mConfig.inputRateHz << ",\n";
        } else {
            file << "  \"recordType\": \"upsampled_output\",\n";
            file << "  \"inputRateHz\": " << mConfig.inputRateHz << ",\n";
            file << "  \"outputRateHz\": " << mConfig.outputRateHz << ",\n";
        }
        file << "  \"totalEvents\": " << mRecordedEvents.size() << ",\n";
        file << "  \"events\": [\n";

        for (size_t i = 0; i < mRecordedEvents.size(); ++i) {
            const auto& event = mRecordedEvents[i];
            auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                event.ts.time_since_epoch()).count();

            file << "    {\n";
            file << "      \"timestamp_ms\": " << timestamp_ms << ",\n";
            file << "      \"x\": " << std::fixed << std::setprecision(2) << event.x << ",\n";
            file << "      \"y\": " << std::fixed << std::setprecision(2) << event.y << ",\n";
            file << "      \"touching\": " << (event.touching ? "true" : "false") << "\n";
            file << "    }";
            if (i < mRecordedEvents.size() - 1) {
                file << ",";
            }
            file << "\n";
        }

        file << "  ]\n";
        file << "}\n";
        file.close();

        std::cout << "Recorded " << mRecordedEvents.size()
                  << " events to: " << mFilePath << std::endl;
    }

    size_t getEventCount() const {
        return mRecordedEvents.size();
    }

    void clear() {
        mRecordedEvents.clear();
    }
};

// --------------------- Touch Device Interface ---------------------
// Abstract interface for touch devices (uinput, mock, etc.)
class TouchDevice {
public:
    virtual ~TouchDevice() = default;
    virtual bool setup(const Config& cfg) = 0;
    virtual void teardown() = 0;
    virtual void emit(const TouchPoint& point) = 0;
};

// --------------------- Touch Device Implementations ---------------------

#ifdef __linux__
// Linux uinput-based touch device
class LinuxTouchDevice : public TouchDevice {
private:
    int mUinputFd = -1;
    bool mPressed  = false;

public:
    bool setup(const Config& cfg) override {
        mUinputFd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (mUinputFd < 0) {
            perror("open /dev/uinput");
            return false;
        }

        // Advertise single-touch absolute pointer
        ioctl(mUinputFd, UI_SET_EVBIT, EV_SYN);
        ioctl(mUinputFd, UI_SET_EVBIT, EV_KEY);
        ioctl(mUinputFd, UI_SET_EVBIT, EV_ABS);

        ioctl(mUinputFd, UI_SET_KEYBIT, BTN_LEFT);
        ioctl(mUinputFd, UI_SET_ABSBIT, ABS_X);
        ioctl(mUinputFd, UI_SET_ABSBIT, ABS_Y);

        struct uinput_user_dev uidev{};
        snprintf(uidev.name, sizeof(uidev.name), "%s", cfg.deviceName.c_str());
        uidev.id.bustype = BUS_USB;
        uidev.id.vendor  = 0x1234;
        uidev.id.product = 0x5678;
        uidev.id.version = 1;

        // Absolute coordinate range
        uidev.absmin[ABS_X] = 0;
        uidev.absmax[ABS_X] = cfg.screenWidth  - 1;
        uidev.absmin[ABS_Y] = 0;
        uidev.absmax[ABS_Y] = cfg.screenHeight - 1;

        if (write(mUinputFd, &uidev, sizeof(uidev)) < 0) {
            perror("write uidev");
            close(mUinputFd);
            mUinputFd = -1;
            return false;
        }
        if (ioctl(mUinputFd, UI_DEV_CREATE) < 0) {
            perror("UI_DEV_CREATE");
            close(mUinputFd);
            mUinputFd = -1;
            return false;
        }

        return true;
    }

    void teardown() override {
        if (mUinputFd >= 0) {
            ioctl(mUinputFd, UI_DEV_DESTROY);
            close(mUinputFd);
            mUinputFd = -1;
        }
    }

    void emit(const TouchPoint& point) override {
        if (mUinputFd < 0) return;

        struct input_event ev{};
        gettimeofday(&ev.time, nullptr);

        // Move pointer
        ev.type = EV_ABS; ev.code = ABS_X; ev.value = static_cast<int>(point.x);
        write(mUinputFd, &ev, sizeof(ev));
        ev.type = EV_ABS; ev.code = ABS_Y; ev.value = static_cast<int>(point.y);
        write(mUinputFd, &ev, sizeof(ev));

        // Button press / release
        bool shouldPress = point.touching;
        if (shouldPress != mPressed) {
            ev.type = EV_KEY; ev.code = BTN_LEFT; ev.value = shouldPress ? 1 : 0;
            write(mUinputFd, &ev, sizeof(ev));
            mPressed = shouldPress;
        }

        // Frame sync
        ev.type = EV_SYN; ev.code = SYN_REPORT; ev.value = 0;
        write(mUinputFd, &ev, sizeof(ev));
    }
};

#endif

// Mock touch device for testing and non-Linux platforms
class MockTouchDevice : public TouchDevice {
private:
    Config mConfig;

public:
    bool setup(const Config& cfg) override {
        mConfig = cfg;
        return true;
    }

    void teardown() override {
        // Clean shutdown
    }

    void emit(const TouchPoint& point) override {
        // Mock device does nothing - events are handled by VirtualTouchDevice's callback system
        (void)point; // Suppress unused parameter warning
    }
};


// Factory function to create appropriate device
std::unique_ptr<TouchDevice> createTouchDevice(const Config& cfg) {
    switch (cfg.deviceType) {
        case DeviceType::Linux:
#ifdef __linux__
            return std::make_unique<LinuxTouchDevice>();
#else
            // Linux device requested but not available, fall back to mock
            return std::make_unique<MockTouchDevice>();
#endif

        case DeviceType::Mock:
        default:
            return std::make_unique<MockTouchDevice>();
    }
}



// --------------------- VirtualTouchDevice::Impl Class ---------------------
class VirtualTouchDevice::Impl {
public:
    explicit Impl(const Config& cfg) : mCfg(cfg) {
        // Pre-allocate processing buffer for efficiency
        // Reserve space for: inputRate * historyDuration + some extra headroom
        size_t expectedSize = static_cast<size_t>(cfg.inputRateHz * cfg.maxInputHistorySec * 1.5);
        mProcessingBuffer.reserve(std::max(expectedSize, size_t(100))); // Minimum 100 elements


        // Create appropriate touch device
        mTouchDevice = createTouchDevice(cfg);

        // Initialize recording functionality if enabled
        if (cfg.enableRawInputRecording) {
            mRawInputRecorder = std::make_unique<FileRecorder>(
                cfg.rawInputRecordPath, cfg, true);
        }

        if (cfg.enableUpsampledRecording) {
            mUpsampledRecorder = std::make_unique<FileRecorder>(
                cfg.upsampledRecordPath,  cfg, false);
        }
    }

    ~Impl() {
        stop();

        // Save recorded data before destruction
        if (mRawInputRecorder) {
            mRawInputRecorder->saveToFile();
        }

        if (mUpsampledRecorder) {
            mUpsampledRecorder->saveToFile();
        }
    }

    bool start() {
        if (mRunning.exchange(true)) return false;
        if (!mTouchDevice || !mTouchDevice->setup(mCfg)) {
            std::cerr << "Failed to setup touch device\n";
            mRunning=false; return false;
        }
        mSenderThread=std::thread(&Impl::senderLoop,this);
        return true;
    }

    void stop() {
        if (!mRunning.exchange(false)) return;
        if (mSenderThread.joinable()) mSenderThread.join();
        if (mTouchDevice) {
            mTouchDevice->teardown();
        }
    }

    void pushInputPoint(const TouchPoint& p) {
        // Validate input point
        if (std::isnan(p.x) || std::isnan(p.y) ||
            p.x < 0 || p.x > mCfg.screenWidth - 1 ||
            p.y < 0 || p.y > mCfg.screenHeight - 1 ) {
            perror("invalid input point");
            return;
        }

        // Record raw input if enabled
        if (mRawInputRecorder) {
            mRawInputRecorder->recordEvent(p);
        }

        {
            std::lock_guard<std::mutex> g(mInputMutex);
            mLatestInput = p;
            mHasNewInput = true;
        }
    }

    // Event callback interface
    void setEventCallback(std::function<void(const TouchPoint&)> callback) {
        mEventCallback = callback;
    }

    // Access to touch device for testing
    TouchDevice* getTouchDevice() const { return mTouchDevice.get(); }

private:
    Config mCfg;
    std::vector<TouchPoint> mProcessingBuffer;  // Only accessed by processing thread

    // Single input point with minimal locking
    std::mutex mInputMutex;
    TouchPoint mLatestInput;
    bool mHasNewInput = false;

    // Touch timeout tracking (processing thread only)
    std::chrono::steady_clock::time_point mLastInputTime;
    bool mHasActiveTouch = false;

    std::atomic<bool> mRunning{false};
    std::thread mSenderThread;

    std::unique_ptr<TouchDevice> mTouchDevice;

    // Event callback
    std::function<void(const TouchPoint&)> mEventCallback;

    // Recording functionality
    std::unique_ptr<FileRecorder> mRawInputRecorder;
    std::unique_ptr<FileRecorder> mUpsampledRecorder;

    bool findBracketing(steady_clock::time_point target, TouchPoint& a, TouchPoint& b) {
        // Lock-free operation - only called from processing thread
        if (mProcessingBuffer.empty()) return false;

        // Need at least 2 points for proper interpolation
        if (mProcessingBuffer.size() < 2) return false;

        if (target <= mProcessingBuffer.front().ts){
            a=b=mProcessingBuffer.front();
            return true;
        }

        if (target >= mProcessingBuffer.back().ts){
            auto& last = mProcessingBuffer.back();

            // Check if we're too far ahead for extrapolation
            double future = toSeconds(target - last.ts);
            auto extrapolationLimit = std::chrono::duration<double, std::milli>(mCfg.maxExtrapolationMs);
            if (future > extrapolationLimit.count() / 1000.0) {
                // Don't generate events beyond extrapolation limit
                return false;
            }

            // Extrapolate if we have velocity information and sufficient confidence
            if (mProcessingBuffer.size() >= 2){
                double confidence = calculateVelocityConfidence(mProcessingBuffer);
                if (confidence > 0.5) { // Only extrapolate with reasonable confidence
                    // Use weighted average of multiple points for better velocity estimation
                    size_t pointsToUse = std::min(size_t(3), mProcessingBuffer.size());
                    double totalWeight = 0.0;
                    double vx = 0.0, vy = 0.0;

                    for (size_t i = 0; i < pointsToUse - 1; ++i) {
                        size_t idx1 = mProcessingBuffer.size() - 1 - i;
                        size_t idx2 = mProcessingBuffer.size() - 2 - i;

                        double dt = toSeconds(mProcessingBuffer[idx1].ts - mProcessingBuffer[idx2].ts);
                        if (dt > 1e-6) {
                            double weight = 1.0 / (1.0 + i); // Recent points have higher weight
                            vx += weight * (mProcessingBuffer[idx1].x - mProcessingBuffer[idx2].x) / dt;
                            vy += weight * (mProcessingBuffer[idx1].y - mProcessingBuffer[idx2].y) / dt;
                            totalWeight += weight;
                        }
                    }

                    if (totalWeight > 0.0) {
                        vx /= totalWeight;
                        vy /= totalWeight;

                        a = last;
                        b = last;
                        b.x = float(last.x + vx * future);
                        b.y = float(last.y + vy * future);
                        b.ts = target;
                        b.touching = last.touching;

                        // Validate extrapolated position is reasonable
                        if (b.x >= -100 && b.x <= mCfg.screenWidth + 100 &&
                            b.y >= -100 && b.y <= mCfg.screenHeight + 100) {
                            return true;
                        }
                    }
                }
            }
            a=b=last;
            return true;
        }

        //should never happen
        std::cout<<"should never happen\n";
        assert(false);

        return false;
    }

    TouchPoint interpolate(const TouchPoint& a, const TouchPoint& b, steady_clock::time_point t) {
        TouchPoint out;
        out.ts = t;

        double denom = toSeconds(b.ts - a.ts);
        double u = (denom <= 1e-6) ? 0.0 : toSeconds(t - a.ts) / denom;

        // Clamp interpolation factor
        u = std::max(0.0, std::min(1.0, u));

        // Interpolate position
        out.x = float((1.0 - u) * a.x + u * b.x);
        out.y = float((1.0 - u) * a.y + u * b.y);

        out.touching = true;

        return out;
    }

    double calculateVelocityConfidence(const std::vector<TouchPoint>& buffer, size_t count = 3) {
        if (buffer.size() < 2) return 0.0;

        // Use std::array with bounds checking
        std::array<double, 10> speeds;
        size_t speedCount = 0;

        size_t pointsToCheck = std::min({count, buffer.size() - 1, speeds.size()});
        double avgSpeed = 0.0;
        double speedVariance = 0.0;

        // Calculate speeds between consecutive points
        for (size_t i = 0; i < pointsToCheck; ++i) {
            size_t idx1 = buffer.size() - 1 - i;
            size_t idx2 = buffer.size() - 2 - i;

            double dt = toSeconds(buffer[idx1].ts - buffer[idx2].ts);
            if (dt > 1e-6 && speedCount < speeds.size()) {
                double dx = buffer[idx1].x - buffer[idx2].x;
                double dy = buffer[idx1].y - buffer[idx2].y;
                double speed = std::sqrt(dx*dx + dy*dy) / dt;
                speeds[speedCount] = speed;
                avgSpeed += speed;
                speedCount++;
            }
        }

        if (speedCount == 0) return 0.0;

        avgSpeed /= speedCount;

        // Calculate variance
        for (size_t i = 0; i < speedCount; ++i) {
            double diff = speeds[i] - avgSpeed;
            speedVariance += diff * diff;
        }
        speedVariance /= speedCount;

        // Confidence is inversely related to coefficient of variation
        if (avgSpeed < 1e-6) return 1.0; // Static point, high confidence

        double cv = std::sqrt(speedVariance) / avgSpeed;
        return std::exp(-cv); // Returns value between 0 and 1
    }

    void emitTouchPoint(const TouchPoint& point) {
        // Record upsampled output if enabled
        if (mUpsampledRecorder) {
            mUpsampledRecorder->recordEvent(point);
        }

        // Call the callback first if installed
        if (mEventCallback) {
            mEventCallback(point);
        }

        // Then emit to the backend device
        if (mTouchDevice) {
            mTouchDevice->emit(point);
        }
    }

    void cleanupOldPoints(steady_clock::time_point /*currentTime*/) {
        if (mProcessingBuffer.size() > 20) {
            mProcessingBuffer.erase(mProcessingBuffer.begin());
        }
    }

    void handleReleaseTouchPoint(const TouchPoint& releasePoint) {
        std::cout<<"handleReleaseTouchPoint \n";
        emitTouchPoint(releasePoint);
        mHasActiveTouch = false;
        mProcessingBuffer.clear();
    }

    void senderLoop() {
        using clk = steady_clock;
        double period = 1.0 / mCfg.outputRateHz;

        while (mRunning) {
            auto currentTick = clk::now();

            // Check for new input and update timestamp to current tick
            bool processNewInput = false;
            TouchPoint newInput;
            {
                std::lock_guard<std::mutex> g(mInputMutex);
                if (mHasNewInput) {
                    newInput = mLatestInput;
                    newInput.ts = currentTick; // Update timestamp to current tick
                    processNewInput = true;
                    mHasNewInput = false;
                    mLastInputTime = currentTick;
                }
            }

            // Handle new input.
            if (processNewInput) {
                if (newInput.touching) {
                    mProcessingBuffer.push_back(newInput);
                    mHasActiveTouch = true;
                    emitTouchPoint(newInput);
                } else {
                    handleReleaseTouchPoint(newInput);
                }
            } else { //handle upsampling if required.
                if (mHasActiveTouch) {
                    auto timeSinceLastInput = toSeconds(currentTick - mLastInputTime);
                    if (timeSinceLastInput >= 0.1 ) {
                        TouchPoint releasePoint = mProcessingBuffer.back();
                        releasePoint.ts = currentTick;
                        releasePoint.touching = false;
                        handleReleaseTouchPoint(releasePoint);
                    } else {
                        if (mProcessingBuffer.size() >= 2) {
                            TouchPoint a, b;
                            if (findBracketing(currentTick, a, b)) {
                                TouchPoint out = interpolate(a, b, currentTick);

                                out.x = std::max(0.0f, std::min(out.x, float(mCfg.screenWidth-1)));
                                out.y = std::max(0.0f, std::min(out.y, float(mCfg.screenHeight-1)));

                                emitTouchPoint(out);
                            }
                        }
                    }
                }

            }

            // Clean up old points periodically
            cleanupOldPoints(currentTick);

            // Wait for next tick using fixed interval
            auto nextTick = currentTick + std::chrono::duration_cast<steady_clock::duration>(duration<double>(period));
            std::this_thread::sleep_until(nextTick);
        }

        // Send final release if needed
        if (mHasActiveTouch && !mProcessingBuffer.empty()) {
            TouchPoint rel = mProcessingBuffer.back();
            rel.ts = clk::now();
            rel.touching = false;
            handleReleaseTouchPoint(rel);
        }
    }

};

// --------------------- VirtualTouchDevice::Impl Implementation ---------------------

// --------------------- VirtualTouchDevice Implementation ---------------------
VirtualTouchDevice::VirtualTouchDevice(const Config& cfg)
    : mImpl(std::make_unique<Impl>(cfg)) {}

VirtualTouchDevice::~VirtualTouchDevice() = default;

bool VirtualTouchDevice::start() {
    return mImpl->start();
}

void VirtualTouchDevice::stop() {
    mImpl->stop();
}

void VirtualTouchDevice::pushInputPoint(const TouchPoint& p) {
    mImpl->pushInputPoint(p);
}

// Event callback interface
void VirtualTouchDevice::setEventCallback(std::function<void(const TouchPoint&)> callback) {
    mImpl->setEventCallback(callback);
}

// --------------------- VirtualTouchDevice::Impl Method Implementations ---------------------

} // namespace vtd
