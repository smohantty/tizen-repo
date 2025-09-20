
#include "VirtualTouchDevice.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <limits>
#include <sstream>
#include <iomanip>
#include <functional>

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

using namespace std::chrono;

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

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
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
public:
    using EventCallback = std::function<void(const TouchPoint&)>;

private:
    Config mConfig;
    EventCallback mEventCallback; // Real-time callback for UI framework simulation

public:
    bool setup(const Config& cfg) override {
        mConfig = cfg;
        return true;
    }

    void teardown() override {
        // Clean shutdown
    }

    void emit(const TouchPoint& point) override {
        // Real-time callback (minimal latency)
        if (mEventCallback) {
            mEventCallback(point);
        }
    }

    // Testing interface
    void setEventCallback(EventCallback callback) { mEventCallback = callback; }
};

// Factory function to create appropriate device
std::unique_ptr<TouchDevice> createTouchDevice() {
#ifdef __linux__
    return std::make_unique<LinuxTouchDevice>();
#else
    return std::make_unique<MockTouchDevice>();
#endif
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
std::unique_ptr<SmoothingStrategy> createSmoothingStrategy(SmoothingType type, const SmoothingConfig& config) {
    switch (type) {
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
    void setSmoothingType(SmoothingType type, const SmoothingConfig& config);

    // Touch transition configuration
    void setTouchTransitionThreshold(double threshold);
    double getTouchTransitionThreshold() const;

    // Access to touch device for testing
    TouchDevice* getTouchDevice() const { return mTouchDevice.get(); }

private:
    Config mCfg;
    std::deque<TouchPoint> mProcessingBuffer;  // Only accessed by processing thread

    // Single input point with minimal locking
    std::mutex mInputMutex;
    TouchPoint mLatestInput;
    bool mHasNewInput = false;

    // Touch timeout tracking (processing thread only)
    std::chrono::steady_clock::time_point mLastInputTime;
    bool mHasActiveTouch = false;

    std::condition_variable mCv;
    std::atomic<bool> mRunning{false};
    std::thread mSenderThread;

    std::unique_ptr<SmoothingStrategy> mSmoother;
    std::unique_ptr<TouchDevice> mTouchDevice;
    bool findBracketing(steady_clock::time_point target,TouchPoint& a,TouchPoint& b);
    TouchPoint interpolate(const TouchPoint& a,const TouchPoint& b,steady_clock::time_point t);
    void senderLoop();
    double calculateVelocityConfidence(const std::deque<TouchPoint>& buffer, size_t count = 3);
};

// --------------------- VirtualTouchDevice::Impl Implementation ---------------------
VirtualTouchDevice::Impl::Impl(const Config& cfg)
    : mCfg(cfg) {
    // Initialize with EMA smoothing by default
    SmoothingConfig smoothConfig;
    smoothConfig.emaAlpha = cfg.emaAlpha;
    mSmoother = createSmoothingStrategy(SmoothingType::EMA, smoothConfig);

    // Create appropriate touch device
    mTouchDevice = createTouchDevice();
}

VirtualTouchDevice::Impl::~Impl() {
    stop();
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

void VirtualTouchDevice::setSmoothingType(SmoothingType type, const SmoothingConfig& config) {
    mImpl->setSmoothingType(type, config);
}

void VirtualTouchDevice::setTouchTransitionThreshold(double threshold) {
    mImpl->setTouchTransitionThreshold(threshold);
}

double VirtualTouchDevice::getTouchTransitionThreshold() const {
    return mImpl->getTouchTransitionThreshold();
}

// Testing interface methods
void VirtualTouchDevice::setMockEventCallback(std::function<void(const TouchPoint&)> callback) {
    if (auto* mockDevice = dynamic_cast<MockTouchDevice*>(mImpl->getTouchDevice())) {
        mockDevice->setEventCallback(callback);
    }
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

    // Minimal lock - just update the latest input point
    {
        std::lock_guard<std::mutex> g(mInputMutex);
        mLatestInput = p;
        mHasNewInput = true;
    }

    // Wake up processing thread
    mCv.notify_one();
}

void VirtualTouchDevice::Impl::setSmoothingType(SmoothingType type, const SmoothingConfig& config) {
    // No mutex needed - smoother is only used by processing thread
    mSmoother = createSmoothingStrategy(type, config);
}

void VirtualTouchDevice::Impl::setTouchTransitionThreshold(double threshold) {
    mCfg.touchTransitionThreshold = std::max(0.0, std::min(0.5, threshold));
}

double VirtualTouchDevice::Impl::getTouchTransitionThreshold() const {
    return mCfg.touchTransitionThreshold;
}


bool VirtualTouchDevice::Impl::findBracketing(steady_clock::time_point target, TouchPoint& a, TouchPoint& b){
    // Lock-free operation - only called from processing thread
    if (mProcessingBuffer.empty()) return false;

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
            a = b = last;
            return true;
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

    // Use binary search for better performance with large buffers
    if (mProcessingBuffer.size() > 10) {
        size_t left = 0, right = mProcessingBuffer.size() - 1;
        while (left < right) {
            size_t mid = left + (right - left) / 2;
            if (mProcessingBuffer[mid].ts < target) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }

        if (left > 0) {
            a = mProcessingBuffer[left - 1];
            b = mProcessingBuffer[left];
            return true;
        } else {
            a = b = mProcessingBuffer.front();
            return true;
        }
    } else {
        // Linear search for small buffers
        for(size_t i=1; i<mProcessingBuffer.size(); ++i){
            if (mProcessingBuffer[i].ts >= target){
                a=mProcessingBuffer[i-1];
                b=mProcessingBuffer[i];
                return true;
            }
        }
    }

    a=b=mProcessingBuffer.back();
    return true;
}

double VirtualTouchDevice::Impl::calculateVelocityConfidence(const std::deque<TouchPoint>& buffer, size_t count) {
    if (buffer.size() < 2) return 0.0;

    size_t pointsToCheck = std::min(count, buffer.size() - 1);
    double avgSpeed = 0.0;
    double speedVariance = 0.0;
    std::vector<double> speeds;

    // Calculate speeds between consecutive points
    for (size_t i = 0; i < pointsToCheck; ++i) {
        size_t idx1 = buffer.size() - 1 - i;
        size_t idx2 = buffer.size() - 2 - i;

        double dt = toSeconds(buffer[idx1].ts - buffer[idx2].ts);
        if (dt > 1e-6) {
            double dx = buffer[idx1].x - buffer[idx2].x;
            double dy = buffer[idx1].y - buffer[idx2].y;
            double speed = std::sqrt(dx*dx + dy*dy) / dt;
            speeds.push_back(speed);
            avgSpeed += speed;
        }
    }

    if (speeds.empty()) return 0.0;

    avgSpeed /= speeds.size();

    // Calculate variance
    for (double speed : speeds) {
        double diff = speed - avgSpeed;
        speedVariance += diff * diff;
    }
    speedVariance /= speeds.size();

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
                }
            } else {
                mProcessingBuffer.push_back(newInput);
                mLastInputTime = newInput.ts;
                mHasActiveTouch = newInput.touching;
            }

            // Clean up old points
            auto cutoff = newInput.ts - duration<double>(mCfg.maxInputHistorySec);
            while(!mProcessingBuffer.empty() && mProcessingBuffer.front().ts < cutoff) {
                mProcessingBuffer.pop_front();
            }
            // Explicit release
            if (!newInput.touching) {
                mHasActiveTouch = false;
                if (mTouchDevice) {
                    mTouchDevice->emit(newInput);
                }
                mProcessingBuffer.clear(); // flush sequence
                std::unique_lock<std::mutex> lk(mInputMutex);
                mCv.wait_until(lk, nextTick, [this](){return !mRunning;});
                continue;
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

                if (mTouchDevice) {
                    mTouchDevice->emit(autoReleasePoint);
                }

                mProcessingBuffer.push_back(autoReleasePoint);
                mHasActiveTouch = false;

                std::unique_lock<std::mutex> lk(mInputMutex);
                mCv.wait_until(lk, nextTick, [this](){return !mRunning;});
                continue;

            }
        }

        // Process output for this tick (no locks needed)
        auto target = nextTick;
        TouchPoint a, b;
        TouchPoint out;

        if(findBracketing(target, a, b)){
            out = interpolate(a, b, target);
            if(mSmoother) {
                out = mSmoother->smooth(out);
            }
            out.x = std::max(0.0f, std::min(out.x, float(mCfg.screenWidth-1)));
            out.y = std::max(0.0f, std::min(out.y, float(mCfg.screenHeight-1)));

            if (mTouchDevice) {
                mTouchDevice->emit(out);
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
        if (mTouchDevice) {
            mTouchDevice->emit(rel);
        }
    }
}
