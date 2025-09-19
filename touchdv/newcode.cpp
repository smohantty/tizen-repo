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
    int mNextTrackingId = 1;
    int mCurrentTrackingId = -1;

public:
    bool setup(const Config& cfg) override {
        mUinputFd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (mUinputFd < 0) {
            perror("open /dev/uinput");
            return false;
        }

        ioctl(mUinputFd, UI_SET_EVBIT, EV_SYN);
        ioctl(mUinputFd, UI_SET_EVBIT, EV_KEY);
        ioctl(mUinputFd, UI_SET_EVBIT, EV_ABS);
        ioctl(mUinputFd, UI_SET_KEYBIT, BTN_TOUCH);
        ioctl(mUinputFd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
        ioctl(mUinputFd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
        ioctl(mUinputFd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
        ioctl(mUinputFd, UI_SET_ABSBIT, ABS_MT_PRESSURE);

        struct uinput_user_dev uidev{};
        snprintf(uidev.name, sizeof(uidev.name), "%s", cfg.deviceName.c_str());
        uidev.id.bustype = BUS_USB;
        uidev.id.vendor = 0x1234;
        uidev.id.product = 0x5678;
        uidev.id.version = 1;
        uidev.absmin[ABS_MT_POSITION_X] = 0;
        uidev.absmax[ABS_MT_POSITION_X] = cfg.screenWidth - 1;
        uidev.absmin[ABS_MT_POSITION_Y] = 0;
        uidev.absmax[ABS_MT_POSITION_Y] = cfg.screenHeight - 1;
        uidev.absmin[ABS_MT_PRESSURE] = 0;
        uidev.absmax[ABS_MT_PRESSURE] = 255;
        uidev.absmin[ABS_MT_TRACKING_ID] = 0;
        uidev.absmax[ABS_MT_TRACKING_ID] = 65535;

        if (write(mUinputFd, &uidev, sizeof(uidev)) < 0) {
            perror("write uidev");
            return false;
        }
        if (ioctl(mUinputFd, UI_DEV_CREATE) < 0) {
            perror("UI_DEV_CREATE");
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

        if (point.touching && mCurrentTrackingId < 0) {
            // Start new touch
            mCurrentTrackingId = mNextTrackingId++;
            if (mNextTrackingId > 65535) mNextTrackingId = 1;
        }

        if (point.touching) {
            // Send touch events
            ev.type = EV_ABS; ev.code = ABS_MT_TRACKING_ID; ev.value = mCurrentTrackingId;
            write(mUinputFd, &ev, sizeof(ev));

            ev.type = EV_ABS; ev.code = ABS_MT_POSITION_X; ev.value = (int)point.x;
            write(mUinputFd, &ev, sizeof(ev));

            ev.type = EV_ABS; ev.code = ABS_MT_POSITION_Y; ev.value = (int)point.y;
            write(mUinputFd, &ev, sizeof(ev));

            ev.type = EV_ABS; ev.code = ABS_MT_PRESSURE; ev.value = point.pressure;
            write(mUinputFd, &ev, sizeof(ev));

            ev.type = EV_SYN; ev.code = SYN_MT_REPORT; ev.value = 0;
            write(mUinputFd, &ev, sizeof(ev));

            ev.type = EV_KEY; ev.code = BTN_TOUCH; ev.value = 1;
            write(mUinputFd, &ev, sizeof(ev));
        } else {
            // End touch
            if (mCurrentTrackingId >= 0) {
                ev.type = EV_ABS; ev.code = ABS_MT_TRACKING_ID; ev.value = -1;
                write(mUinputFd, &ev, sizeof(ev));

                ev.type = EV_SYN; ev.code = SYN_MT_REPORT; ev.value = 0;
                write(mUinputFd, &ev, sizeof(ev));

                ev.type = EV_KEY; ev.code = BTN_TOUCH; ev.value = 0;
                write(mUinputFd, &ev, sizeof(ev));

                mCurrentTrackingId = -1;
            }
        }

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
    EventCallback mEventCallback;

public:
    bool setup(const Config& cfg) override {
        mConfig = cfg;
        return true;
    }

    void teardown() override {}

    void emit(const TouchPoint& point) override {
        if (mEventCallback) {
            mEventCallback(point);
        }
    }

    void setEventCallback(EventCallback callback) { mEventCallback = callback; }
};

std::unique_ptr<TouchDevice> createTouchDevice() {
#ifdef __linux__
    return std::make_unique<LinuxTouchDevice>();
#else
    return std::make_unique<MockTouchDevice>();
#endif
}

// --------------------- Smoothing Strategies ---------------------
class SmoothingStrategy {
public:
    virtual ~SmoothingStrategy() = default;
    virtual TouchPoint smooth(const TouchPoint& in) = 0;
    virtual void reset() {}
};

class EmaSmoother : public SmoothingStrategy {
    bool mInitialized = false;
    float mX = 0, mY = 0;
    double mAlpha;
public:
    explicit EmaSmoother(double alpha) : mAlpha(alpha) {}
    TouchPoint smooth(const TouchPoint& in) override;
    void reset() override;
};

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

class NoSmoothing : public SmoothingStrategy {
public:
    TouchPoint smooth(const TouchPoint& in) override { return in; }
};

std::unique_ptr<SmoothingStrategy> createSmoothingStrategy(SmoothingType type, const SmoothingConfig& config) {
    switch (type) {
        case SmoothingType::None: return std::make_unique<NoSmoothing>();
        case SmoothingType::EMA: return std::make_unique<EmaSmoother>(config.emaAlpha);
        case SmoothingType::Kalman: return std::make_unique<KalmanSmoother>(config.kalmanQ, config.kalmanR);
        case SmoothingType::OneEuro:
            return std::make_unique<OneEuroSmoother>(config.oneEuroFreq, config.oneEuroMinCutoff,
                                                     config.oneEuroBeta, config.oneEuroDCutoff);
        default: return std::make_unique<NoSmoothing>();
    }
}

// --------------------- Smoother Implementations ---------------------
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
    mX += mVX*dt; mY += mVY*dt;
    for(int i=0;i<4;i++) mP[i][i]+=mQ;

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

// --------------------- VirtualTouchDevice::Impl ---------------------
class VirtualTouchDevice::Impl {
public:
    explicit Impl(const Config& cfg);
    ~Impl();

    bool start();
    void stop();
    void pushInputPoint(const TouchPoint& p);
    void setSmoothingType(SmoothingType type, const SmoothingConfig& config);

    void setTouchTransitionThreshold(double threshold);
    double getTouchTransitionThreshold() const;

    TouchDevice* getTouchDevice() const { return mTouchDevice.get(); }

private:
    Config mCfg;
    std::deque<TouchPoint> mProcessingBuffer;

    std::mutex mInputMutex;
    TouchPoint mLatestInput;
    bool mHasNewInput = false;

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

VirtualTouchDevice::Impl::Impl(const Config& cfg)
    : mCfg(cfg) {
    SmoothingConfig smoothConfig;
    smoothConfig.emaAlpha = cfg.emaAlpha;
    mSmoother = createSmoothingStrategy(SmoothingType::EMA, smoothConfig);
    mTouchDevice = createTouchDevice();
}
VirtualTouchDevice::Impl::~Impl() { stop(); }

// --------------------- VirtualTouchDevice ---------------------
VirtualTouchDevice::VirtualTouchDevice(const Config& cfg)
    : mImpl(std::make_unique<Impl>(cfg)) {}
VirtualTouchDevice::~VirtualTouchDevice() = default;
bool VirtualTouchDevice::start() { return mImpl->start(); }
void VirtualTouchDevice::stop() { mImpl->stop(); }
void VirtualTouchDevice::pushInputPoint(const TouchPoint& p) { mImpl->pushInputPoint(p); }
void VirtualTouchDevice::setSmoothingType(SmoothingType type, const SmoothingConfig& config) { mImpl->setSmoothingType(type, config); }
void VirtualTouchDevice::setTouchTransitionThreshold(double threshold) { mImpl->setTouchTransitionThreshold(threshold); }
double VirtualTouchDevice::getTouchTransitionThreshold() const { return mImpl->getTouchTransitionThreshold(); }
void VirtualTouchDevice::setMockEventCallback(std::function<void(const TouchPoint&)> callback) {
    if (auto* mockDevice = dynamic_cast<MockTouchDevice*>(mImpl->getTouchDevice())) {
        mockDevice->setEventCallback(callback);
    }
}

// --------------------- Impl methods ---------------------
void VirtualTouchDevice::Impl::senderLoop() {
    using clk = steady_clock;

    // Output period (e.g. 1/30kHz = 33.3µs) from config
    const double periodSec = 1.0 / mCfg.outputRateHz;

    // First tick aligned to "now"
    auto nextTick = clk::now();

    while (mRunning) {
        auto start = clk::now();
        nextTick = start + duration_cast<clk::duration>(duration<double>(periodSec));

        // ------------------------------
        // 1. Grab latest input (producer side)
        // ------------------------------
        TouchPoint latestInput;
        bool hasInput = false;
        {
            std::lock_guard<std::mutex> lock(mInputMutex);
            if (mHasNewInput) {
                latestInput = mLatestInput;
                mHasNewInput = false;
                hasInput = true;
                mLastInputTime = latestInput.ts;

                // Add to buffer for bracketing
                mProcessingBuffer.push_back(latestInput);
                // Limit buffer size (keep recent ~20 points)
                if (mProcessingBuffer.size() > 20) {
                    mProcessingBuffer.pop_front();
                }
            }
        }

        // ------------------------------
        // 2. Determine output sample
        // ------------------------------
        TouchPoint outputPoint;
        outputPoint.ts = clk::now();

        if (mProcessingBuffer.empty()) {
            // Case A: No input at all → no touch
            outputPoint.touching = false;
            outputPoint.pressure = 0;
        } else {
            // Case B: We have input history
            TouchPoint a, b;
            if (findBracketing(outputPoint.ts, a, b)) {
                // Interpolation case
                outputPoint = interpolate(a, b, outputPoint.ts);
                outputPoint.touching = a.touching && b.touching;
            } else {
                // Extrapolation case: use last sample
                outputPoint = mProcessingBuffer.back();
                outputPoint.ts = clk::now();
            }
        }

        // ------------------------------
        // 3. Apply smoothing
        // ------------------------------
        if (mSmoother) {
            outputPoint = mSmoother->smooth(outputPoint);
        }

        // ------------------------------
        // 4. Velocity & Direction adjustment
        // ------------------------------
        // If buffer has at least 2 points, compute simple velocity
        if (mProcessingBuffer.size() >= 2) {
            const auto& p1 = mProcessingBuffer[mProcessingBuffer.size() - 2];
            const auto& p2 = mProcessingBuffer.back();

            double dt = toSeconds(p2.ts - p1.ts);
            if (dt > 0) {
                double vx = (p2.x - p1.x) / dt;
                double vy = (p2.y - p1.y) / dt;

                // Adjust current point by projected velocity
                double dtPredict = toSeconds(outputPoint.ts - p2.ts);
                outputPoint.x += static_cast<float>(vx * dtPredict);
                outputPoint.y += static_cast<float>(vy * dtPredict);
            }
        }

        // ------------------------------
        // 5. Apply touch transition threshold
        // ------------------------------
        if (!outputPoint.touching && mHasActiveTouch) {
            // Only end the touch if no input for threshold duration
            auto idleTime = clk::now() - mLastInputTime;
            if (toSeconds(idleTime) < mCfg.touchTransitionThreshold) {
                outputPoint.touching = true;
                outputPoint.pressure = mProcessingBuffer.back().pressure;
            } else {
                mHasActiveTouch = false;
            }
        } else if (outputPoint.touching && !mHasActiveTouch) {
            // Touch has just started
            mHasActiveTouch = true;
        }

        // ------------------------------
        // 6. Emit event to device
        // ------------------------------
        if (mTouchDevice) {
            mTouchDevice->emit(outputPoint);
        }

        
        // ------------------------------
        // 7. Sleep until next tick
        // ------------------------------
        std::unique_lock<std::mutex> lk(mInputMutex);
        mCv.wait_until(lk, nextTick, [this]() { return !mRunning; });
    }

    // ------------------------------
    // Cleanup: ensure touch release if still active
    // ------------------------------
    if (mHasActiveTouch && !mProcessingBuffer.empty()) {
        TouchPoint release;
        release.ts = clk::now();
        release.x = mProcessingBuffer.back().x;
        release.y = mProcessingBuffer.back().y;
        release.pressure = 0;
        release.touching = false;
        if (mTouchDevice) {
            mTouchDevice->emit(release);
        }
    }
}

