#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <memory>
#include <limits>
#include <mutex>
#include <atomic>

namespace perf {

// Simple data structure for returning performance metrics values
struct PerformanceMetricsData {
    double mMinDurationMs = std::numeric_limits<double>::max();
    double mMaxDurationMs = 0.0;
    double mTotalDurationMs = 0.0;
    double mAvgDurationMs = 0.0;
    size_t mCallCount = 0;
};

class PerformanceMetrics {
public:
    // Constructor and Destructor
    PerformanceMetrics();
    ~PerformanceMetrics();

    // Non-copyable and non-movable
    PerformanceMetrics(const PerformanceMetrics&) = delete;
    PerformanceMetrics& operator=(const PerformanceMetrics&) = delete;
    PerformanceMetrics(PerformanceMetrics&&) = delete;
    PerformanceMetrics& operator=(PerformanceMetrics&&) = delete;

    // Public interface
    void update(double durationMs);
    void reset();

    // Getters for accessing data
    double getMinDurationMs() const;
    double getMaxDurationMs() const;
    double getTotalDurationMs() const;
    double getAvgDurationMs() const;
    size_t getCallCount() const;

    // Methods for group aggregation (internal use)
    void addToGroup(size_t callCount, double totalDuration, double minDuration, double maxDuration);

    // Method to get data as a copyable struct
    PerformanceMetricsData getData() const;

private:
    // PIMPL idiom - hide implementation details
    class Impl;
    Impl* mImpl;
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
    PerformanceMetricsData getMetrics(const std::string& functionName) const;
    std::vector<std::pair<std::string, PerformanceMetricsData>> getAllMetrics() const;

    // Simple reporting
    std::string generateReport() const;

    // Real-time monitoring
    void startRealTimeMonitoring(std::chrono::milliseconds interval = std::chrono::milliseconds(1000));
    void stopRealTimeMonitoring();
    std::string getRealTimeMonitoringFilePath() const;

    // Function grouping
    void addFunctionToGroup(const std::string& groupName, const std::string& functionName);
    PerformanceMetricsData getGroupMetrics(const std::string& groupName) const;

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