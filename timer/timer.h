#pragma once

#include <chrono>
#include <functional>
#include <memory>

class Timer {
public:
    using Callback = std::function<void()>;
    using Duration = std::chrono::milliseconds;

private:
    // Forward declaration of implementation class
    class Impl;
    std::unique_ptr<Impl> pImpl;

public:
    Timer();
    ~Timer();

    // Explicitly defaulted move operations (copy is implicitly deleted due to unique_ptr)
    Timer(Timer&&) noexcept;
    Timer& operator=(Timer&&) noexcept;

    // Add a timer that expires after the specified duration
    uint64_t addTimer(const Duration& duration, Callback callback);

    // Add a repeating timer that fires repeatedly at the specified interval
    uint64_t addRepeatingTimer(const Duration& interval, Callback callback);

    // Add a timer that expires after the specified duration in seconds
    uint64_t addTimer(const std::chrono::seconds& duration, Callback callback);

    // Add a repeating timer that fires repeatedly at the specified interval in seconds
    uint64_t addRepeatingTimer(const std::chrono::seconds& interval, Callback callback);

    // Remove a specific timer by ID (useful for stopping repeating timers)
    bool removeTimer(uint64_t timer_id);

    // Check for expired timers and trigger them
    void update();

    // Get the number of active timers
    size_t getActiveTimerCount() const;

    // Check if there are any active timers
    bool hasActiveTimers() const;

    // Get time until next timer expires (returns 0 if no timers or timer already expired)
    Duration getTimeToNextTimer() const;
};
