#pragma once

#include <chrono>
#include <memory>
#include <string>

namespace perf {

class PerfMonitor {
public:
    // Singleton pattern for global access
    static PerfMonitor& getInstance();


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

    // Simple reporting
    std::string generateReport() const;

    // Real-time monitoring
    void startRealTimeMonitoring(std::chrono::milliseconds interval = std::chrono::milliseconds(1000));
    void stopRealTimeMonitoring();
    std::string getRealTimeMonitoringFilePath() const;

    // Reset functionality
    void reset();
    void resetFunction(const std::string& functionName);

private:
    // Private constructor and destructor for singleton
    PerfMonitor();
    ~PerfMonitor();

    // Internal helper method for ScopedTimer
    void updateMetrics(const std::string& functionName, double durationMs);

    // PIMPL idiom - hide implementation details
    class Impl;
    std::unique_ptr<Impl> mImpl;
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