#include "TouchUpscaler.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

#ifdef __linux__
#include <fcntl.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#endif

using namespace std::chrono;
namespace {
struct TouchPoint {
    steady_clock::time_point timestamp;
    float x{0.0f};
    float y{0.0f};
    bool valid{false};
};

struct TouchInput {
    int x{0};
    int y{0};
    bool isDown{false};
};

class InputBackend {
   public:
    virtual ~InputBackend() = default;
    virtual bool setup(const vtd::Config& cfg) = 0;
    virtual void teardown() = 0;
    virtual void emit(const TouchInput& point) = 0;
};

class SingleTouchDevice : public InputBackend {
   private:
    int mUinputFd = -1;
    bool mPressed = false;

   public:
    bool setup(const vtd::Config& cfg) override {
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
        uidev.id.vendor = 0x1234;
        uidev.id.product = 0x5678;
        uidev.id.version = 1;

        // Absolute coordinate range
        uidev.absmin[ABS_X] = 0;
        uidev.absmax[ABS_X] = cfg.screenWidth - 1;
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

    void emit(const TouchInput& point) override {
        if (mUinputFd < 0)
            return;

        struct input_event ev{};
        gettimeofday(&ev.time, nullptr);

        // Move pointer
        ev.type = EV_ABS;
        ev.code = ABS_X;
        ev.value = point.x;
        write(mUinputFd, &ev, sizeof(ev));
        ev.type = EV_ABS;
        ev.code = ABS_Y;
        ev.value = point.y;
        write(mUinputFd, &ev, sizeof(ev));

        // Button press / release
        bool shouldPress = point.isDown;
        if (shouldPress != mPressed) {
            ev.type = EV_KEY;
            ev.code = BTN_LEFT;
            ev.value = shouldPress ? 1 : 0;
            write(mUinputFd, &ev, sizeof(ev));
            mPressed = shouldPress;
        }

        // Frame sync
        ev.type = EV_SYN;
        ev.code = SYN_REPORT;
        ev.value = 0;
        write(mUinputFd, &ev, sizeof(ev));
    }
};

class MockInputBackend : public InputBackend {
   private:
    vtd::Config mConfig;

   public:
    bool setup(const vtd::Config& cfg) override {
        mConfig = cfg;
        return true;
    }

    void teardown() override {
        // Clean shutdown
    }

    void emit(const TouchInput& point) override { (void) point; }
};

}  // namespace

namespace vtd {

Config Config::getDefault() {
    return Config{};
}

struct TouchUpscaler::Impl {
    enum class State { Idle, Active, MaybeUp };

    Config mCfg;

    std::mutex mMutex;
    std::atomic<bool> mRunning{false};
    std::thread mWorkerThread;
    std::unique_ptr<InputBackend> mBackend;

    State mState{State::Idle};
    std::vector<TouchPoint> mHistory;

    std::mutex mInputMutex;
    TouchPoint mLatestInput;
    bool mHasNewInput = false;

    Impl(const Config& cfg) : mCfg(cfg) {
        mHistory.reserve(10);
        switch (cfg.backend) {
            case Backend::SingleTouchDevice:
                mBackend = std::make_unique<SingleTouchDevice>();
                break;
            case Backend::Mock:
            default:
                mBackend = std::make_unique<MockInputBackend>();
                break;
        }
    }

    ~Impl() {
        stop();
        if (mBackend) {
            mBackend->teardown();
        }
    }

    void start() {
        if (mRunning.exchange(true)) {
            return;
        }

        mRunning = true;
        mBackend->setup(mCfg);
        mWorkerThread = std::thread(&Impl::runLoop, this);
    }

    void stop() {
        mRunning = false;
        if (mWorkerThread.joinable()) {
            mWorkerThread.join();
        }
    }

    void push(const TouchSample& raw) {
        TouchPoint point = {.timestamp = steady_clock::now(), .x = raw.x, .y = raw.y, .valid = raw.valid};
        {
            std::lock_guard<std::mutex> g(mInputMutex);
            mLatestInput = point;
            mHasNewInput = true;
        }
    }

   private:
    void runLoop() {
        double period = 1.0 / mCfg.outputHz;
        while (mRunning) {
            auto currentTick = steady_clock::now();

            // 1. Check for new input
            TouchPoint input;
            bool hasInput = false;
            {
                std::lock_guard<std::mutex> lk(mInputMutex);
                if (mHasNewInput) {
                    input = mLatestInput;
                    mHasNewInput = false;
                    hasInput = true;
                }
            }

            // 2. Update history if input is valid
            if (hasInput && input.valid) {
                if (mHistory.size() >= mCfg.historySize) {
                    mHistory.erase(mHistory.begin());
                } else {
                    mHistory.push_back(input);
                }
            }

            // 3. Generate upscale output
            TouchPoint out;
            if (updateState(out)) {
                // send it to the backend
            }

            auto nextTick = currentTick + std::chrono::duration_cast<steady_clock::duration>(duration<double>(period));
            std::this_thread::sleep_until(nextTick);
        }
    }

    bool updateState(TouchPoint& out) { return false; }
};

TouchUpscaler::TouchUpscaler(const Config& cfg) : mImpl(std::make_unique<Impl>(cfg)) {
}

TouchUpscaler::~TouchUpscaler() = default;

void TouchUpscaler::start() {
    mImpl->start();
}

void TouchUpscaler::stop() {
    mImpl->stop();
}

void TouchUpscaler::push(const TouchSample& p) {
    mImpl->push(p);
}

}  // namespace vtd
