
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

        std::ofstream file(mFilePath);
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
            file << "  \"smoothingType\": \"" << static_cast<int>(mConfig.smoothingType) << "\",\n";
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

// --------------------- Smoothing Strategy Classes (Internal) ---------------------
class SmoothingStrategy {
public:
    virtual ~SmoothingStrategy() = default;
    virtual TouchPoint smooth(const TouchPoint& in) = 0;
    virtual void reset() {}
};

// EMA Smoothing Implementation
class EmaSmoother : public SmoothingStrategy {
    bool mInitialized = false;
    float mX = 0, mY = 0;
    double mAlpha;
public:
    explicit EmaSmoother(double alpha) : mAlpha(alpha) {}
    TouchPoint smooth(const TouchPoint& in) override;
    void reset() override;
};

// Kalman Smoothing Implementation
class KalmanSmoother : public SmoothingStrategy {
    bool mInitialized = false;
    double mX=0, mVX=0, mY=0, mVY=0;
    double mP[4][4]{};
    double mQ, mR;
    steady_clock::time_point mLast;
public:
    KalmanSmoother(double q, double r) : mQ(q), mR(r) {}
    TouchPoint smooth(const TouchPoint& in) override;
    void reset() override;
};

// OneEuro Smoothing Implementation
class OneEuroSmoother : public SmoothingStrategy {
    bool mInitialized=false;
    float mX=0,mY=0,mDX=0,mDY=0;
    double mFreq, mMinCutoff, mBeta, mDCutoff;
    steady_clock::time_point mLast;
    static double alpha(double cutoff,double dt);
    static double lowpass(double x,double prev,double a);
public:
    OneEuroSmoother(double freq, double minCutoff, double beta, double dCutoff)
        : mFreq(freq), mMinCutoff(minCutoff), mBeta(beta), mDCutoff(dCutoff) {}
    TouchPoint smooth(const TouchPoint& in) override;
    void reset() override;
};

// No Smoothing Implementation
class NoSmoothing : public SmoothingStrategy {
public:
    TouchPoint smooth(const TouchPoint& in) override;
};

// Factory function to create smoothing strategies
std::unique_ptr<SmoothingStrategy> createSmoothingStrategy(const Config& config) {
    switch (config.smoothingType) {
        case SmoothingType::None:
            return std::make_unique<NoSmoothing>();
        case SmoothingType::EMA:
            return std::make_unique<EmaSmoother>(config.emaAlpha);
        case SmoothingType::Kalman:
            return std::make_unique<KalmanSmoother>(config.kalmanQ, config.kalmanR);
        case SmoothingType::OneEuro:
            return std::make_unique<OneEuroSmoother>(config.oneEuroFreq, config.oneEuroMinCutoff,
                                                   config.oneEuroBeta, config.oneEuroDCutoff);
        default:
            return std::make_unique<NoSmoothing>();
    }
}

// --------------------- Smoothing Strategy Implementations ---------------------
TouchPoint EmaSmoother::smooth(const TouchPoint& in) {
    TouchPoint out = in;
    if (!mInitialized) {
        mX = in.x; mY = in.y;
        mInitialized = true;
    } else {
        mX = float(mAlpha*in.x + (1.0-mAlpha)*mX);
        mY = float(mAlpha*in.y + (1.0-mAlpha)*mY);
    }
    out.x = mX; out.y = mY;
    return out;
}

void EmaSmoother::reset() { mInitialized=false; }

TouchPoint KalmanSmoother::smooth(const TouchPoint& in) {
    auto now = in.ts;
    double dt = mInitialized ? toSeconds(now - mLast) : 1.0/120.0;
    if (dt<=0) dt=1.0/120.0;
    mLast = now;

    if (!mInitialized) {
        mX=in.x; mY=in.y; mVX=0; mVY=0;
        for(int i=0;i<4;i++) for(int j=0;j<4;j++) mP[i][j]=0;
        mInitialized = true;
        return in;
    }
    // Predict
    mX += mVX*dt; mY += mVY*dt;
    for(int i=0;i<4;i++) mP[i][i]+=mQ;

    // Update with measurement
    double zx=in.x, zy=in.y;
    double yx=zx-mX, yy=zy-mY;
    double Sx=mP[0][0]+mR, Sy=mP[2][2]+mR;
    double Kx=mP[0][0]/Sx, Ky=mP[2][2]/Sy;

    mX += Kx*yx; mY += Ky*yy;
    mVX += Kx*yx/dt; mVY += Ky*yy/dt;
    mP[0][0]*=(1-Kx); mP[2][2]*=(1-Ky);

    TouchPoint out=in;
    out.x=float(mX); out.y=float(mY);
    return out;
}

void KalmanSmoother::reset() { mInitialized=false; }

double OneEuroSmoother::alpha(double cutoff,double dt) {
    double tau=1.0/(2*M_PI*cutoff);
    return 1.0/(1.0+tau/dt);
}

double OneEuroSmoother::lowpass(double x,double prev,double a) {
    return a*x+(1-a)*prev;
}

TouchPoint OneEuroSmoother::smooth(const TouchPoint& in) {
    auto now=in.ts;
    double dt = mInitialized ? toSeconds(now-mLast) : 1.0/mFreq;
    if (dt<=0) dt=1.0/mFreq;
    mLast=now;

    if (!mInitialized) {
        mX=in.x; mY=in.y; mDX=0; mDY=0;
        mInitialized=true;
        return in;
    }
    double dx=(in.x-mX)/dt, dy=(in.y-mY)/dt;
    double adx=alpha(mDCutoff,dt);
    mDX=float(lowpass(dx,mDX,adx));
    mDY=float(lowpass(dy,mDY,adx));

    double cutoffX=mMinCutoff+mBeta*std::fabs(mDX);
    double cutoffY=mMinCutoff+mBeta*std::fabs(mDY);
    double ax=alpha(cutoffX,dt), ay=alpha(cutoffY,dt);

    mX=float(lowpass(in.x,mX,ax));
    mY=float(lowpass(in.y,mY,ay));

    TouchPoint out=in; out.x=mX; out.y=mY;
    return out;
}

void OneEuroSmoother::reset() { mInitialized=false; }

TouchPoint NoSmoothing::smooth(const TouchPoint& in) { return in; }

// --------------------- VirtualTouchDevice::Impl Class ---------------------
class VirtualTouchDevice::Impl {
public:
    explicit Impl(const Config& cfg);
    ~Impl();

    bool start();
    void stop();
    void pushInputPoint(const TouchPoint& p);

    // Event callback interface
    void setEventCallback(std::function<void(const TouchPoint&)> callback);

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
    bool mHasEmittedRelease = false;  // Track if release event has been emitted for current sequence

    std::condition_variable mCv;
    std::atomic<bool> mRunning{false};
    std::thread mSenderThread;

    std::unique_ptr<SmoothingStrategy> mSmoother;
    std::unique_ptr<TouchDevice> mTouchDevice;

    // Event callback
    std::function<void(const TouchPoint&)> mEventCallback;

    // Recording functionality
    std::unique_ptr<FileRecorder> mRawInputRecorder;
    std::unique_ptr<FileRecorder> mUpsampledRecorder;

    bool findBracketing(steady_clock::time_point target,TouchPoint& a,TouchPoint& b);
    TouchPoint interpolate(const TouchPoint& a,const TouchPoint& b,steady_clock::time_point t);
    void senderLoop();
    double calculateVelocityConfidence(const std::vector<TouchPoint>& buffer, size_t count = 3);
    void emitTouchPoint(const TouchPoint& point); // Helper to emit with callback
};

// --------------------- VirtualTouchDevice::Impl Implementation ---------------------
VirtualTouchDevice::Impl::Impl(const Config& cfg)
    : mCfg(cfg) {
    // Pre-allocate processing buffer for efficiency
    // Reserve space for: inputRate * historyDuration + some extra headroom
    size_t expectedSize = static_cast<size_t>(cfg.inputRateHz * cfg.maxInputHistorySec * 1.5);
    mProcessingBuffer.reserve(std::max(expectedSize, size_t(100))); // Minimum 100 elements

    // Initialize smoothing from config
    mSmoother = createSmoothingStrategy(cfg);

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

VirtualTouchDevice::Impl::~Impl() {
    stop();

    // Save recorded data before destruction
    if (mRawInputRecorder) {
        mRawInputRecorder->saveToFile();
    }

    if (mUpsampledRecorder) {
        mUpsampledRecorder->saveToFile();
    }
}

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
bool VirtualTouchDevice::Impl::start() {
    if (mRunning.exchange(true)) return false;
    if (!mTouchDevice || !mTouchDevice->setup(mCfg)) {
        std::cerr << "Failed to setup touch device\n";
        mRunning=false; return false;
    }
    mSenderThread=std::thread(&Impl::senderLoop,this);
    return true;
}

void VirtualTouchDevice::Impl::stop() {
    if (!mRunning.exchange(false)) return;
    mCv.notify_all();
    if (mSenderThread.joinable()) mSenderThread.join();
    if (mTouchDevice) {
        mTouchDevice->teardown();
    }
}

void VirtualTouchDevice::Impl::pushInputPoint(const TouchPoint& p) {
    // Validate input point
    if (std::isnan(p.x) || std::isnan(p.y) ||
        p.x < -1000 || p.x > mCfg.screenWidth + 1000 ||
        p.y < -1000 || p.y > mCfg.screenHeight + 1000) {
        // Invalid point, ignore it
        return;
    }

    // Record raw input if enabled
    if (mRawInputRecorder) {
        mRawInputRecorder->recordEvent(p);
    }

    // Minimal lock - just update the latest input point
    {
        std::lock_guard<std::mutex> g(mInputMutex);
        mLatestInput = p;
        mHasNewInput = true;
    }

    // Wake up processing thread
    mCv.notify_one();
}

void VirtualTouchDevice::Impl::setEventCallback(std::function<void(const TouchPoint&)> callback) {
    mEventCallback = callback;
}

void VirtualTouchDevice::Impl::emitTouchPoint(const TouchPoint& point) {
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


bool VirtualTouchDevice::Impl::findBracketing(steady_clock::time_point target, TouchPoint& a, TouchPoint& b){
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

    for(size_t i=1; i<mProcessingBuffer.size(); ++i){
        if (mProcessingBuffer[i].ts >= target){
            a=mProcessingBuffer[i-1];
            b=mProcessingBuffer[i];
            return true;
        }
    }

    a=b=mProcessingBuffer.back();
    return true;
}

double VirtualTouchDevice::Impl::calculateVelocityConfidence(const std::vector<TouchPoint>& buffer, size_t count) {
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

TouchPoint VirtualTouchDevice::Impl::interpolate(const TouchPoint& a,const TouchPoint& b,steady_clock::time_point t){
    TouchPoint out;
    out.ts = t;

    double denom = toSeconds(b.ts - a.ts);
    double u = (denom <= 1e-6) ? 0.0 : toSeconds(t - a.ts) / denom;

    // Clamp interpolation factor
    u = std::max(0.0, std::min(1.0, u));

    // Interpolate position
    out.x = float((1.0 - u) * a.x + u * b.x);
    out.y = float((1.0 - u) * a.y + u * b.y);

    // Intelligent touch state interpolation optimized for discrete sequences (TTTTTR pattern)
    if (a.touching == b.touching) {
        // Same state, keep it
        out.touching = a.touching;
    } else {
        // State transition - use intelligent thresholds for discrete touch sequences
        double threshold = mCfg.touchTransitionThreshold;
        double releaseThreshold = 1.0 - mCfg.touchTransitionThreshold;

        if (a.touching && !b.touching) {
            // Touch to release: delay release to ensure clean gesture completion
            // This handles cases where IR might miss the final release
            out.touching = (u < releaseThreshold);
        } else if (!a.touching && b.touching) {
            // Release to touch: quick touch activation for responsive gestures
            out.touching = (u >= threshold);
        } else {
            // Fallback for any other state combinations
            out.touching = (u >= 0.5);
        }
    }

    return out;
}

void VirtualTouchDevice::Impl::senderLoop(){
    using clk=steady_clock;
    double period=1.0/mCfg.outputRateHz;
    auto nextTick=clk::now();

    while(mRunning){
        nextTick += std::chrono::duration_cast<steady_clock::duration>(duration<double>(period));

        // Check for new input with minimal lock time
        bool hasNewInput = false;
        TouchPoint newInput;
        {
            std::lock_guard<std::mutex> g(mInputMutex);
            if (mHasNewInput) {
                newInput = mLatestInput;
                hasNewInput = true;
                mHasNewInput = false;
            }
        }

        // If we have new input, add it to processing buffer (no lock needed)
        if (hasNewInput) {
            // Validate timestamp ordering - reject points too far in the past
            if (!mProcessingBuffer.empty()) {
                auto timeDiff = toSeconds(newInput.ts - mProcessingBuffer.back().ts);
                if (timeDiff >= -0.1) { // Not more than 100ms in the past
                    mProcessingBuffer.push_back(newInput);
                    mLastInputTime = newInput.ts;
                    mHasActiveTouch = newInput.touching;

                    // Emit the new input point immediately (no smoothing for raw events)
                    // Exception: Don't emit release events immediately - let interpolation handle them
                    if (newInput.touching) {
                        TouchPoint emitPoint = newInput;
                        // Don't apply smoothing to raw input events - they should be sent as-is
                        emitPoint.x = std::max(0.0f, std::min(emitPoint.x, float(mCfg.screenWidth-1)));
                        emitPoint.y = std::max(0.0f, std::min(emitPoint.y, float(mCfg.screenHeight-1)));
                        emitTouchPoint(emitPoint);
                    }
                }
            } else {
                mProcessingBuffer.push_back(newInput);
                mLastInputTime = newInput.ts;
                mHasActiveTouch = newInput.touching;

                // Emit the first input point immediately (no smoothing for raw events)
                // Exception: Don't emit release events immediately - let interpolation handle them
                if (newInput.touching) {
                    TouchPoint emitPoint = newInput;
                    // Don't apply smoothing to raw input events - they should be sent as-is
                    emitPoint.x = std::max(0.0f, std::min(emitPoint.x, float(mCfg.screenWidth-1)));
                    emitPoint.y = std::max(0.0f, std::min(emitPoint.y, float(mCfg.screenHeight-1)));
                    emitTouchPoint(emitPoint);
                }
            }

            // Clean up old points
            auto cutoff = newInput.ts - duration<double>(mCfg.maxInputHistorySec);
            auto it = mProcessingBuffer.begin();
            while (it != mProcessingBuffer.end() && it->ts < cutoff) {
                ++it;
            }
            if (it != mProcessingBuffer.begin()) {
                mProcessingBuffer.erase(mProcessingBuffer.begin(), it);
            }

            // Update active touch state for release events
            if (!newInput.touching) {
                mHasActiveTouch = false;
            } else {
                // New touch sequence starting, reset release flag
                mHasEmittedRelease = false;
            }
        }

        // Check for touch timeout and auto-release if needed
        if (mCfg.touchTimeoutMs > 0.0 && mHasActiveTouch && !mProcessingBuffer.empty()) {
            auto currentTime = nextTick; // Use same time as processing target
            auto timeSinceLastInput = toSeconds(currentTime - mLastInputTime);

            if (timeSinceLastInput * 1000.0 > mCfg.touchTimeoutMs) {
                // Force release due to timeout
                TouchPoint autoReleasePoint = mProcessingBuffer.back();
                autoReleasePoint.touching = false;
                autoReleasePoint.ts = currentTime;

                emitTouchPoint(autoReleasePoint);

                mProcessingBuffer.push_back(autoReleasePoint);
                mHasActiveTouch = false;
                mHasEmittedRelease = true;

                std::unique_lock<std::mutex> lk(mInputMutex);
                mCv.wait_until(lk, nextTick, [this](){return !mRunning;});
                continue;

            }
        }

        // Process output for this tick (no locks needed)
        auto target = nextTick;
        TouchPoint a, b;
        TouchPoint out;

        // Try to find bracketing points for interpolation
        if(findBracketing(target, a, b)){
            out = interpolate(a, b, target);

            // Emit events based on touch state and release tracking
            bool shouldEmit = false;

            if (out.touching) {
                // Always emit touching events
                shouldEmit = true;
            } else if (!mHasEmittedRelease) {
                // Only emit the first release event for this sequence
                shouldEmit = true;
                mHasEmittedRelease = true;
            }

            if (shouldEmit) {
                if(mSmoother) {
                    out = mSmoother->smooth(out);
                }
                out.x = std::max(0.0f, std::min(out.x, float(mCfg.screenWidth-1)));
                out.y = std::max(0.0f, std::min(out.y, float(mCfg.screenHeight-1)));

                emitTouchPoint(out);
            }
        } else if (!mProcessingBuffer.empty()) {
            // If we can't find bracketing points but have data, emit the latest point as-is
            // This handles the case where we have 1 or 2 points but can't interpolate yet
            auto& latestPoint = mProcessingBuffer.back();
            TouchPoint emitPoint = latestPoint;

            // Only emit if this is very close to the actual input timing to avoid duplicates
            auto timeDiff = toSeconds(target - latestPoint.ts);
            if (timeDiff >= -0.01 && timeDiff < 0.01) { // Within 10ms of the actual input
                bool shouldEmit = false;

                if (emitPoint.touching) {
                    // Always emit touching events
                    shouldEmit = true;
                } else if (!mHasEmittedRelease) {
                    // Only emit the first release event for this sequence
                    shouldEmit = true;
                    mHasEmittedRelease = true;
                }

                if (shouldEmit) {
                    if(mSmoother) {
                        emitPoint = mSmoother->smooth(emitPoint);
                    }
                    emitPoint.x = std::max(0.0f, std::min(emitPoint.x, float(mCfg.screenWidth-1)));
                    emitPoint.y = std::max(0.0f, std::min(emitPoint.y, float(mCfg.screenHeight-1)));

                    emitTouchPoint(emitPoint);
                }
            }
        }

        // Clear buffer if touch sequence is complete (no active touch and no new input expected)
        if (!mHasActiveTouch && !mProcessingBuffer.empty()) {
            // Check if the last point in buffer is a release event and enough time has passed
            auto lastPoint = mProcessingBuffer.back();
            if (!lastPoint.touching) {
                auto timeSinceLastPoint = toSeconds(target - lastPoint.ts);
                // Clear buffer after 100ms of inactivity following a release
                if (timeSinceLastPoint > 0.1) {
                    mProcessingBuffer.clear();
                    mHasEmittedRelease = false;  // Reset for next sequence
                }
            }
        }

        // Wait until next scheduled tick (minimal lock for condition variable)
        std::unique_lock<std::mutex> lk(mInputMutex);
        mCv.wait_until(lk, nextTick, [this](){return !mRunning;});
    }

    // Send final release event only if there's an active touch
    if (mHasActiveTouch && !mProcessingBuffer.empty()) {
        TouchPoint rel;
        rel.ts = clk::now();
        rel.x = mProcessingBuffer.back().x;
        rel.y = mProcessingBuffer.back().y;
        rel.touching = false;
        emitTouchPoint(rel);
    }
}

} // namespace vtd
