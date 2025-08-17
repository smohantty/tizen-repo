#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <memory>
#include <limits>
#include <mutex>
#include <atomic>

namespace perf {

struct PerformanceMetrics {
    double mMinDurationMs = std::numeric_limits<double>::max();
    double mMaxDurationMs = 0.0;
    double mTotalDurationMs = 0.0;
    double mAvgDurationMs = 0.0;
    std::atomic<size_t> mCallCount{0};
    mutable std::mutex mMutex; // For coordinated updates

    void update(double durationMs);
    void reset();

    // Copy constructor and assignment operator
    PerformanceMetrics() = default;
    PerformanceMetrics(const PerformanceMetrics& other);
    PerformanceMetrics& operator=(const PerformanceMetrics& other);
};

class PerfMonitor {
public:
    // Singleton pattern for global access
    static PerfMonitor& getInstance();

    // Constructor and Destructor
    PerfMonitor();
    ~PerfMonitor();

    // Non-copyable
    PerfMonitor(const PerfMonitor&) = delete;
    PerfMonitor& operator=(const PerfMonitor&) = delete;

    // Core measurement APIs
    void startMeasurement(const std::string& functionName);
    void endMeasurement(const std::string& functionName);

    // Template for measuring any callable
    template<typename Func, typename... Args>
    auto measureCall(const std::string& functionName, Func&& func, Args&&... args)
        -> decltype(func(args...)) {
        auto startTime = std::chrono::steady_clock::now();
        auto result = func(std::forward<Args>(args)...);
        auto endTime = std::chrono::steady_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime).count() / 1000.0;

        // Forward to implementation
        updateMetrics(functionName, duration);
        return result;
    }

    // Scoped measurement (RAII)
    class ScopedTimer {
    public:
        ScopedTimer(const std::string& functionName);
        ~ScopedTimer();
    private:
        std::string mName;
        std::chrono::steady_clock::time_point mStartTime;
    };

    // Data retrieval
    PerformanceMetrics getMetrics(const std::string& functionName) const;
    std::vector<std::pair<std::string, PerformanceMetrics>> getAllMetrics() const;

    // Simple reporting
    std::string generateReport() const;

    // Real-time monitoring
    void startRealTimeMonitoring(std::chrono::milliseconds interval = std::chrono::milliseconds(1000));
    void stopRealTimeMonitoring();
    std::string getRealTimeMonitoringFilePath() const;

    // Function grouping
    void addFunctionToGroup(const std::string& groupName, const std::string& functionName);
    PerformanceMetrics getGroupMetrics(const std::string& groupName) const;

    // Reset functionality
    void reset();
    void resetFunction(const std::string& functionName);

    // Helper method for template
    void updateMetrics(const std::string& functionName, double durationMs);

private:
    // PIMPL idiom - hide implementation details
    class Impl;
    Impl* mImpl;
};

// Convenience macros for easy integration
#define PERF_MEASURE_SCOPE(name) \
    perf::PerfMonitor::ScopedTimer __perf_timer__(name)

#define PERF_MEASURE_FUNCTION() \
    perf::PerfMonitor::ScopedTimer __perf_timer__(__FUNCTION__)

#define PERF_START(name) \
    perf::PerfMonitor::getInstance().startMeasurement(name)

#define PERF_END(name) \
    perf::PerfMonitor::getInstance().endMeasurement(name)

} // namespace perf