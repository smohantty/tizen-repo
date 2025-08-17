#include "timer.h"
#include <algorithm>
#include <queue>
#include <vector>

// Implementation class definition
class Timer::Impl {
public:
    using TimePoint = std::chrono::steady_clock::time_point;

    struct TimerNode {
        TimePoint expiry_time;
        Timer::Callback callback;
        uint64_t timer_id;
        bool is_repeating;
        Timer::Duration repeat_interval;

        TimerNode(TimePoint expiry, Timer::Callback cb, uint64_t id, bool repeating = false, Timer::Duration interval = Timer::Duration::zero())
            : expiry_time(expiry), callback(std::move(cb)), timer_id(id), is_repeating(repeating), repeat_interval(interval) {}
    };

        struct TimerComparator {
        bool operator()(const std::unique_ptr<TimerNode>& a, const std::unique_ptr<TimerNode>& b) const {
            // Min-heap: return true if a should come after b
            return a->expiry_time > b->expiry_time;
        }
    };

    using TimerHeap = std::priority_queue<std::unique_ptr<TimerNode>,
                                          std::vector<std::unique_ptr<TimerNode>>,
                                          TimerComparator>;

            TimerHeap timer_heap;
    uint64_t next_timer_id;

    Impl() : next_timer_id(1) {}
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
    auto timer_node = std::make_unique<Impl::TimerNode>(expiry_time, std::move(callback), pImpl->next_timer_id, false);

    pImpl->timer_heap.push(std::move(timer_node));

    return pImpl->next_timer_id++;
}

uint64_t Timer::addRepeatingTimer(const Duration& interval, Callback callback) {
    auto current_time = std::chrono::steady_clock::now();
    auto expiry_time = current_time + interval;
    auto timer_node = std::make_unique<Impl::TimerNode>(expiry_time, std::move(callback), pImpl->next_timer_id, true, interval);

    pImpl->timer_heap.push(std::move(timer_node));

    return pImpl->next_timer_id++;
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

    while (!pImpl->timer_heap.empty()) {
        auto timer = std::move(const_cast<std::unique_ptr<Impl::TimerNode>&>(pImpl->timer_heap.top()));
        pImpl->timer_heap.pop();

        if (timer->timer_id == timer_id) {
            found = true;
            // Don't add this timer back
        } else {
            remaining_timers.push_back(std::move(timer));
        }
    }

    // Rebuild the heap with remaining timers
    for (auto& timer : remaining_timers) {
        pImpl->timer_heap.push(std::move(timer));
    }

    return found;
}

void Timer::update() {
    // Get current time for comparison
    auto current_time = std::chrono::steady_clock::now();

    // Store repeating timers that need to be re-added
    std::vector<std::unique_ptr<Impl::TimerNode>> repeating_timers_to_readd;

    // Process all expired timers
    while (!pImpl->timer_heap.empty()) {
        const auto& next_timer = pImpl->timer_heap.top();

        // Check if this timer has expired
        if (next_timer->expiry_time <= current_time) {
            // Call the callback function before removing from heap
            if (next_timer->callback) {
                next_timer->callback();
            }

            // If it's a repeating timer, prepare to re-add it
            if (next_timer->is_repeating) {
                auto new_expiry = current_time + next_timer->repeat_interval;
                auto new_timer = std::make_unique<Impl::TimerNode>(
                    new_expiry,
                    next_timer->callback,
                    next_timer->timer_id,
                    true,
                    next_timer->repeat_interval
                );
                repeating_timers_to_readd.push_back(std::move(new_timer));
            }

            // Timer has expired, remove it from heap
            pImpl->timer_heap.pop();
        } else {
            // This timer (and all subsequent timers) haven't expired yet
            break;
        }
    }

    // Re-add repeating timers
    for (auto& timer : repeating_timers_to_readd) {
        pImpl->timer_heap.push(std::move(timer));
    }
}

size_t Timer::getActiveTimerCount() const {
    return pImpl->timer_heap.size();
}

bool Timer::hasActiveTimers() const {
    return !pImpl->timer_heap.empty();
}

Timer::Duration Timer::getTimeToNextTimer() const {
    if (pImpl->timer_heap.empty()) {
        return Duration::zero();
    }

    auto current_time = std::chrono::steady_clock::now();
    auto next_expiry = pImpl->timer_heap.top()->expiry_time;
    if (next_expiry <= current_time) {
        return Duration::zero(); // Timer has already expired
    }

    return std::chrono::duration_cast<Duration>(next_expiry - current_time);
}
