#include "timer.h"
#include <algorithm>
#include <queue>
#include <vector>

// Implementation class definition
class Timer::Impl {
public:
    using TimePoint = std::chrono::steady_clock::time_point;

    struct TimerNode {
        TimePoint mExpiryTime;
        Timer::Callback mCallback;
        uint64_t mTimerId;
        bool mIsRepeating;
        Timer::Duration mRepeatInterval;

        TimerNode(TimePoint expiry, Timer::Callback cb, uint64_t id, bool repeating = false, Timer::Duration interval = Timer::Duration::zero())
            : mExpiryTime(expiry), mCallback(std::move(cb)), mTimerId(id), mIsRepeating(repeating), mRepeatInterval(interval) {}
    };

        struct TimerComparator {
        bool operator()(const std::unique_ptr<TimerNode>& a, const std::unique_ptr<TimerNode>& b) const {
            // Min-heap: return true if a should come after b
            return a->mExpiryTime > b->mExpiryTime;
        }
    };

    using TimerHeap = std::priority_queue<std::unique_ptr<TimerNode>,
                                          std::vector<std::unique_ptr<TimerNode>>,
                                          TimerComparator>;

    TimerHeap mTimerHeap;
    uint64_t mNextTimerId;

    Impl() : mNextTimerId(1) {}
};

// Timer class implementation
Timer::Timer() : pImpl(std::make_unique<Impl>()) {
}

Timer::~Timer() = default;

Timer::Timer(Timer&&) noexcept = default;

Timer& Timer::operator=(Timer&&) noexcept = default;

uint64_t Timer::addTimer(const Duration& duration, Callback callback) {
    auto current_time = std::chrono::steady_clock::now();
    auto expiry_time = current_time + duration;
    auto timer_node = std::make_unique<Impl::TimerNode>(expiry_time, std::move(callback), pImpl->mNextTimerId, false);

    pImpl->mTimerHeap.push(std::move(timer_node));

    return pImpl->mNextTimerId++;
}

uint64_t Timer::addRepeatingTimer(const Duration& interval, Callback callback) {
    auto current_time = std::chrono::steady_clock::now();
    auto expiry_time = current_time + interval;
    auto timer_node = std::make_unique<Impl::TimerNode>(expiry_time, std::move(callback), pImpl->mNextTimerId, true, interval);

    pImpl->mTimerHeap.push(std::move(timer_node));

    return pImpl->mNextTimerId++;
}

uint64_t Timer::addTimer(const std::chrono::seconds& duration, Callback callback) {
    auto duration_ms = std::chrono::duration_cast<Duration>(duration);
    return addTimer(duration_ms, std::move(callback));
}

uint64_t Timer::addRepeatingTimer(const std::chrono::seconds& interval, Callback callback) {
    auto interval_ms = std::chrono::duration_cast<Duration>(interval);
    return addRepeatingTimer(interval_ms, std::move(callback));
}

bool Timer::removeTimer(uint64_t timer_id) {
    // Since std::priority_queue doesn't support arbitrary removal,
    // we'll rebuild the heap without the target timer
    std::vector<std::unique_ptr<Impl::TimerNode>> remaining_timers;
    bool found = false;

    while (!pImpl->mTimerHeap.empty()) {
        auto timer = std::move(const_cast<std::unique_ptr<Impl::TimerNode>&>(pImpl->mTimerHeap.top()));
        pImpl->mTimerHeap.pop();

        if (timer->mTimerId == timer_id) {
            found = true;
            // Don't add this timer back
        } else {
            remaining_timers.push_back(std::move(timer));
        }
    }

    // Rebuild the heap with remaining timers
    for (auto& timer : remaining_timers) {
        pImpl->mTimerHeap.push(std::move(timer));
    }

    return found;
}

void Timer::update() {
    // Get current time for comparison
    auto current_time = std::chrono::steady_clock::now();

    // Store expired timers to execute their callbacks after heap manipulation
    std::vector<Callback> callbacks_to_execute;

    // Store repeating timers that need to be re-added
    std::vector<std::unique_ptr<Impl::TimerNode>> repeating_timers_to_readd;

    // First pass: collect all expired timers and prepare repeating timers
    while (!pImpl->mTimerHeap.empty()) {
        const auto& next_timer = pImpl->mTimerHeap.top();

        // Check if this timer has expired
        if (next_timer->mExpiryTime <= current_time) {
            // Store the callback for later execution
            if (next_timer->mCallback) {
                callbacks_to_execute.push_back(next_timer->mCallback);
            }

            // If it's a repeating timer, prepare to re-add it
            if (next_timer->mIsRepeating) {
                auto new_expiry = current_time + next_timer->mRepeatInterval;
                auto new_timer = std::make_unique<Impl::TimerNode>(
                    new_expiry,
                    next_timer->mCallback,
                    next_timer->mTimerId,
                    true,
                    next_timer->mRepeatInterval
                );
                repeating_timers_to_readd.push_back(std::move(new_timer));
            }

            // Timer has expired, remove it from heap
            pImpl->mTimerHeap.pop();
        } else {
            // This timer (and all subsequent timers) haven't expired yet
            break;
        }
    }

    // Re-add repeating timers before executing callbacks
    for (auto& timer : repeating_timers_to_readd) {
        pImpl->mTimerHeap.push(std::move(timer));
    }

    // Second pass: execute all callbacks after heap manipulation is complete
    // This ensures that any new timers added in callbacks won't interfere with current update
    for (const auto& callback : callbacks_to_execute) {
        callback();
    }
}

size_t Timer::getActiveTimerCount() const {
    return pImpl->mTimerHeap.size();
}

bool Timer::hasActiveTimers() const {
    return !pImpl->mTimerHeap.empty();
}

Timer::Duration Timer::getTimeToNextTimer() const {
    if (pImpl->mTimerHeap.empty()) {
        return Duration::zero();
    }

    auto current_time = std::chrono::steady_clock::now();
    auto next_expiry = pImpl->mTimerHeap.top()->mExpiryTime;
    if (next_expiry <= current_time) {
        return Duration::zero(); // Timer has already expired
    }

    return std::chrono::duration_cast<Duration>(next_expiry - current_time);
}
