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

// Factory function to create appropriate device
std::unique_ptr<TouchDevice> createTouchDevice() {
#ifdef __linux__
    return std::make_unique<LinuxTouchDevice>();
#else
    return std::make_unique<MockTouchDevice>();
#endif
}

// --------------------- VirtualTouchDevice::Impl::senderLoop ---------------------
void VirtualTouchDevice::Impl::senderLoop(){
    using clk=steady_clock;
    double period=1.0/mCfg.outputRateHz;
    auto nextTick=clk::now();

    while(mRunning){
        nextTick += std::chrono::duration_cast<steady_clock::duration>(duration<double>(period));

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

        if (hasNewInput) {
            if (!mProcessingBuffer.empty()) {
                auto timeDiff = toSeconds(newInput.ts - mProcessingBuffer.back().ts);
                if (timeDiff >= -0.1) { 
                    mProcessingBuffer.push_back(newInput);
                    mLastInputTime = newInput.ts;
                    mHasActiveTouch = newInput.touching;
                }
            } else {
                mProcessingBuffer.push_back(newInput);
                mLastInputTime = newInput.ts;
                mHasActiveTouch = newInput.touching;
            }

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

        // Timeout release
        if (mCfg.touchTimeoutMs > 0.0 && mHasActiveTouch && !mProcessingBuffer.empty()) {
            auto currentTime = nextTick; 
            auto timeSinceLastInput = toSeconds(currentTime - mLastInputTime);

            if (timeSinceLastInput * 1000.0 > mCfg.touchTimeoutMs) {
                TouchPoint autoReleasePoint = mProcessingBuffer.back();
                autoReleasePoint.touching = false;
                autoReleasePoint.pressure = 0;
                autoReleasePoint.ts = currentTime;

                if (mTouchDevice) {
                    mTouchDevice->emit(autoReleasePoint);
                }
                mProcessingBuffer.clear(); // flush after timeout release
                mHasActiveTouch = false;

                std::unique_lock<std::mutex> lk(mInputMutex);
                mCv.wait_until(lk, nextTick, [this](){return !mRunning;});
                continue;
            }
        }

        // Interpolation
        auto target = nextTick;
        TouchPoint a, b, out;
        if(findBracketing(target, a, b)){
            out = interpolate(a, b, target);
            if(mSmoother) out = mSmoother->smooth(out);

            out.x = std::max(0.0f, std::min(out.x, float(mCfg.screenWidth-1)));
            out.y = std::max(0.0f, std::min(out.y, float(mCfg.screenHeight-1)));

            if (mTouchDevice) {
                mTouchDevice->emit(out);
            }
        }

        std::unique_lock<std::mutex> lk(mInputMutex);
        mCv.wait_until(lk, nextTick, [this](){return !mRunning;});
    }

    // Final forced release
    if (mHasActiveTouch && !mProcessingBuffer.empty()) {
        TouchPoint rel;
        rel.ts = clk::now();
        rel.x = mProcessingBuffer.back().x;
        rel.y = mProcessingBuffer.back().y;
        rel.pressure = 0;
        rel.touching = false;
        if (mTouchDevice) {
            mTouchDevice->emit(rel);
        }
        mProcessingBuffer.clear();
    }
}
