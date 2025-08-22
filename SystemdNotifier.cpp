#include "SystemdNotifier.h"
#include <systemd/sd-daemon.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <condition_variable>

namespace systemd {

class SystemdNotifier::Impl {
public:
    Impl(){
    }

    ~Impl() {
        stop();
    }


    void start() {
        if (sd_notify(0, "READY=1") < 0) {
            std::cerr << "Failed to notify systemd of service start" << std::endl;
        }

        mWorkerThread = std::thread(&Impl::heartbeatWorker, this);
    }

    void stop() {
        mStopRequested = true;

        if (mWorkerThread.joinable()) {
            mWorkerThread.join();
        }

        // Notify systemd that the service is stopping
        if (sd_notify(0, "STOPPING=1") < 0) {
            std::cerr << "Failed to notify systemd of service stop" << std::endl;
        }
    }

private:
    std::chrono::seconds getHeartbeatInterval() {
        uint64_t timeout_usec;
        std::chrono::seconds heartbeat_interval;

        if (sd_watchdog_enabled(0, &timeout_usec) > 0) {
            heartbeat_interval = std::chrono::microseconds(timeout_usec / 2);
        } else {
            // No watchdog enabled, use a reasonable default
            heartbeat_interval = std::chrono::seconds(30);
        }

        return heartbeat_interval;
    }

    void heartbeatWorker() {
        std::chrono::seconds heartbeat_interval = getHeartbeatInterval();

        while (!mStopRequested) {
            std::this_thread::sleep_for(heartbeat_interval);

            if (sd_notify(0, "WATCHDOG=1") < 0) {
                std::cerr << "Failed to send systemd heartbeat" << std::endl;
                break;
            }
        }
    }

    std::atomic<bool> mStopRequested;
    std::thread mWorkerThread;
};


SystemdNotifier::SystemdNotifier() : mImpl(std::make_unique<Impl>()) {
}

SystemdNotifier::~SystemdNotifier() = default;

void SystemdNotifier::start() {
    mImpl->start();
}

void SystemdNotifier::stop() {
    mImpl->stop();
}


} // namespace systemd
