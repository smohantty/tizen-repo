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

        TimerNode(TimePoint expiry, Timer::Callback cb, uint64_t id)
            : expiry_time(expiry), callback(std::move(cb)), timer_id(id) {}
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
    auto timer_node = std::make_unique<Impl::TimerNode>(expiry_time, std::move(callback), pImpl->next_timer_id);

    pImpl->timer_heap.push(std::move(timer_node));

    return pImpl->next_timer_id++;
}

uint64_t Timer::addTimer(const std::chrono::seconds& duration, Callback callback) {
    auto duration_ms = std::chrono::duration_cast<Duration>(duration);
    return addTimer(duration_ms, std::move(callback));
}

void Timer::update() {
    // Get current time for comparison
    auto current_time = std::chrono::steady_clock::now();

        // Process all expired timers
    while (!pImpl->timer_heap.empty()) {
        const auto& next_timer = pImpl->timer_heap.top();

        // Check if this timer has expired
        if (next_timer->expiry_time <= current_time) {
            // Call the callback function before removing from heap
            if (next_timer->callback) {
                next_timer->callback();
            }

            // Timer has expired, remove it from heap
            pImpl->timer_heap.pop();
        } else {
            // This timer (and all subsequent timers) haven't expired yet
            break;
        }
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
