#include "PerfMonitor.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <fstream>
#include <mutex>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace perf {

// PerformanceMetrics - simple POD type with grouped data
struct PerformanceMetrics {
    // Grouped data structure
    struct Data {
        double minDurationMs = std::numeric_limits<double>::max();
        double maxDurationMs = 0.0;
        double totalDurationMs = 0.0;
        double avgDurationMs = 0.0;
        size_t callCount = 0;
    };

    Data mData;                    // All metrics data grouped together
    mutable std::mutex mMutex;     // Protects mData

    // Methods for internal use
    void update(double durationMs);
    void reset();
    void addToGroup(size_t callCount, double totalDuration, double minDuration, double maxDuration);

    // Efficient method to get all data with single lock
    Data getAllData() const;
};

// PerformanceMetrics implementation

void PerformanceMetrics::update(double durationMs) {
    std::lock_guard<std::mutex> lock(mMutex);

    size_t currentCount = mData.callCount;
    if (currentCount == 0) {
        mData.minDurationMs = durationMs;
        mData.maxDurationMs = durationMs;
    } else {
        mData.minDurationMs = std::min(mData.minDurationMs, durationMs);
        mData.maxDurationMs = std::max(mData.maxDurationMs, durationMs);
    }

    // Update total and count
    mData.totalDurationMs += durationMs;
    mData.callCount++;
    size_t newCount = mData.callCount;

    // Update average
    mData.avgDurationMs = mData.totalDurationMs / newCount;
}

void PerformanceMetrics::reset() {
    std::lock_guard<std::mutex> lock(mMutex);
    mData = Data{};  // Reset to default values
}

void PerformanceMetrics::addToGroup(size_t callCount, double totalDuration, double minDuration, double maxDuration) {
    std::lock_guard<std::mutex> lock(mMutex);

    size_t currentCount = mData.callCount;
    size_t newCount = currentCount + callCount;

    mData.callCount = newCount;
    mData.totalDurationMs += totalDuration;

    if (currentCount == 0) {
        mData.minDurationMs = minDuration;
        mData.maxDurationMs = maxDuration;
    } else {
        mData.minDurationMs = std::min(mData.minDurationMs, minDuration);
        mData.maxDurationMs = std::max(mData.maxDurationMs, maxDuration);
    }

    if (newCount > 0) {
        mData.avgDurationMs = mData.totalDurationMs / newCount;
    }
}

PerformanceMetrics::Data PerformanceMetrics::getAllData() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mData;  // Simple copy of the grouped data
}



// PIMPL implementation class
class PerfMonitor::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    // Internal data structures with m-prefix camelCase naming
    std::unordered_map<std::string, std::unique_ptr<PerformanceMetrics>> mFunctionMetrics;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> mActiveMeasurements;

    // Mutexes for thread safety
    mutable std::mutex mFunctionMetricsMutex;
    mutable std::mutex mActiveMeasurementsMutex;

    // Real-time monitoring with m-prefix camelCase naming
    std::atomic<bool> mRealTimeMonitoring{false};
    std::thread mRealTimeThread;
    std::chrono::milliseconds mMonitoringInterval{1000};
    std::string mMonitoringFilePath;

    // Helper methods
    void realTimeMonitoringLoop();
    std::string getTempFilePath() const;
    void stopRealTimeMonitoring();
};

void PerfMonitor::Impl::realTimeMonitoringLoop() {
    while (mRealTimeMonitoring.load()) {
        std::ofstream file(mMonitoringFilePath); // Remove std::ios::app to overwrite
        if (file.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                now.time_since_epoch()).count();

            // Write header information
            file << "=== PerfMonitor Real-time Performance Log ===\n";
            file << "Monitor this file with: tail -f " << mMonitoringFilePath << "\n";
            file << "Last updated at timestamp: " << timestamp << "\n\n";

            // Generate current statistics report with shared lock for reading
            // Simple struct for monitoring data
            struct MonitorData {
                double minDurationMs;
                double maxDurationMs;
                double totalDurationMs;
                double avgDurationMs;
                size_t callCount;
            };

            size_t totalCalls = 0;
            double totalExecutionTime = 0.0;
            size_t functionCount = 0;

            // Create a copy of the metrics to minimize lock time
            std::unordered_map<std::string, MonitorData> metricsCopy;
            {
                std::lock_guard<std::mutex> lock(mFunctionMetricsMutex);
                for (const auto& pair : mFunctionMetrics) {
                    if (pair.second) {  // Check if unique_ptr is valid
                        const auto& metrics = *pair.second;
                        auto data = metrics.getAllData();
                        metricsCopy[pair.first] = {
                            data.minDurationMs,
                            data.maxDurationMs,
                            data.totalDurationMs,
                            data.avgDurationMs,
                            data.callCount
                        };
                    }
                }
                functionCount = mFunctionMetrics.size();
            }

            for (const auto& pair : metricsCopy) {
                totalCalls += pair.second.callCount;
                totalExecutionTime += pair.second.totalDurationMs;
            }

            file << "=== Current Performance Statistics ===\n\n";
            file << "Total Functions Monitored: " << functionCount << "\n";
            file << "Total Function Calls: " << totalCalls << "\n";
            file << "Total Execution Time: " << std::fixed << std::setprecision(3)
                 << totalExecutionTime << " ms\n\n";

            file << std::left << std::setw(45) << "Function Name"
                 << std::setw(10) << "Calls"
                 << std::setw(12) << "Avg (ms)"
                 << std::setw(12) << "Min (ms)"
                 << std::setw(12) << "Max (ms)"
                 << std::setw(12) << "Total (ms)" << "\n";
            file << std::string(103, '-') << "\n";

            for (const auto& pair : metricsCopy) {
                const auto& metrics = pair.second;
                file << std::left << std::setw(45) << pair.first
                     << std::setw(10) << metrics.callCount
                     << std::setw(12) << std::fixed << std::setprecision(3) << metrics.avgDurationMs
                     << std::setw(12) << std::fixed << std::setprecision(3) << metrics.minDurationMs
                     << std::setw(12) << std::fixed << std::setprecision(3) << metrics.maxDurationMs
                     << std::setw(12) << std::fixed << std::setprecision(3) << metrics.totalDurationMs << "\n";
            }

            file.close();
        }
        std::this_thread::sleep_for(mMonitoringInterval);
    }
}

std::string PerfMonitor::Impl::getTempFilePath() const {
    std::string tempDir = "/tmp";
    std::string filename = "/perfmonitor_" + std::to_string(getpid()) + ".log";
    std::cout << "Temp file path: " << tempDir + filename << std::endl;
    return tempDir + filename;
}

void PerfMonitor::Impl::stopRealTimeMonitoring() {
    mRealTimeMonitoring.store(false);
    if (mRealTimeThread.joinable()) {
        mRealTimeThread.join();
    }
}

// PerfMonitor implementation
PerfMonitor::PerfMonitor() : mImpl(std::make_unique<Impl>()) {
}

PerfMonitor::~PerfMonitor() {
    if (mImpl) {
        stopRealTimeMonitoring();
    }
}

PerfMonitor& PerfMonitor::getInstance() {
    static PerfMonitor instance;
    return instance;
}

void PerfMonitor::startMeasurement(const std::string& functionName) {
    std::lock_guard<std::mutex> lock(mImpl->mActiveMeasurementsMutex);
    mImpl->mActiveMeasurements[functionName] = std::chrono::steady_clock::now();
}

void PerfMonitor::endMeasurement(const std::string& functionName) {
    auto endTime = std::chrono::steady_clock::now();

    std::chrono::steady_clock::time_point startTime;
    bool foundMeasurement = false;

    // Find and remove from active measurements
    {
        std::lock_guard<std::mutex> lock(mImpl->mActiveMeasurementsMutex);
        auto it = mImpl->mActiveMeasurements.find(functionName);
        if (it != mImpl->mActiveMeasurements.end()) {
            startTime = it->second;
            foundMeasurement = true;
            mImpl->mActiveMeasurements.erase(it);
        }
    }

    if (foundMeasurement) {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime).count() / 1000.0;

        // Update metrics with lock
        {
            std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
            auto& metrics = mImpl->mFunctionMetrics[functionName];
            if (!metrics) {
                metrics = std::make_unique<PerformanceMetrics>();
            }
            metrics->update(duration);
        }
    }
}

void PerfMonitor::updateMetrics(const std::string& functionName, double durationMs) {
    std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
    auto& metrics = mImpl->mFunctionMetrics[functionName];
    if (!metrics) {
        metrics = std::make_unique<PerformanceMetrics>();
    }
    metrics->update(durationMs);
}

PerfMonitor::ScopedTimer::ScopedTimer(const std::string& functionName)
    : mName(functionName), mStartTime(std::chrono::steady_clock::now()) {
}

PerfMonitor::ScopedTimer::~ScopedTimer() {
    try {
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - mStartTime).count() / 1000.0;

        auto& monitor = PerfMonitor::getInstance();
        if (monitor.mImpl) {
            monitor.updateMetrics(mName, duration);
        }
    } catch (...) {
        // Ignore exceptions during destruction
    }
}



std::string PerfMonitor::generateReport() const {
    std::ostringstream report;

    // Simple struct for report data
    struct ReportData {
        double minDurationMs;
        double maxDurationMs;
        double totalDurationMs;
        double avgDurationMs;
        size_t callCount;
    };

    size_t totalCalls = 0;
    double totalExecutionTime = 0.0;
    size_t functionCount = 0;

    // Create a copy of the metrics to minimize lock time
    std::unordered_map<std::string, ReportData> metricsCopy;
    {
        std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
        for (const auto& pair : mImpl->mFunctionMetrics) {
            if (pair.second) {  // Check if unique_ptr is valid
                const auto& metrics = *pair.second;
                auto data = metrics.getAllData();
                metricsCopy[pair.first] = {
                    data.minDurationMs,
                    data.maxDurationMs,
                    data.totalDurationMs,
                    data.avgDurationMs,
                    data.callCount
                };
            }
        }
        functionCount = mImpl->mFunctionMetrics.size();
    }

    for (const auto& pair : metricsCopy) {
        totalCalls += pair.second.callCount;
        totalExecutionTime += pair.second.totalDurationMs;
    }

    report << "=== Performance Monitor Report ===\n\n";
    report << "Total Functions Monitored: " << functionCount << "\n";
    report << "Total Function Calls: " << totalCalls << "\n";
    report << "Total Execution Time: " << std::fixed << std::setprecision(3)
           << totalExecutionTime << " ms\n\n";

    report << std::left << std::setw(45) << "Function Name"
           << std::setw(10) << "Calls"
           << std::setw(12) << "Avg (ms)"
           << std::setw(12) << "Min (ms)"
           << std::setw(12) << "Max (ms)"
           << std::setw(12) << "Total (ms)" << "\n";
    report << std::string(103, '-') << "\n";

    for (const auto& pair : metricsCopy) {
        const auto& metrics = pair.second;
        report << std::left << std::setw(45) << pair.first
               << std::setw(10) << metrics.callCount
               << std::setw(12) << std::fixed << std::setprecision(3) << metrics.avgDurationMs
               << std::setw(12) << std::fixed << std::setprecision(3) << metrics.minDurationMs
               << std::setw(12) << std::fixed << std::setprecision(3) << metrics.maxDurationMs
               << std::setw(12) << std::fixed << std::setprecision(3) << metrics.totalDurationMs << "\n";
    }

    return report.str();
}

void PerfMonitor::startRealTimeMonitoring(std::chrono::milliseconds interval) {
    if (mImpl->mRealTimeMonitoring.load()) {
        return;
    }

    mImpl->mMonitoringInterval = interval;
    mImpl->mMonitoringFilePath = mImpl->getTempFilePath();

    // Create initial file with header
    std::ofstream file(mImpl->mMonitoringFilePath);
    if (file.is_open()) {
        file << "=== PerfMonitor Real-time Performance Log ===\n";
        file << "Monitor this file with: tail -f " << mImpl->mMonitoringFilePath << "\n";
        file << "Real-time monitoring started...\n\n";
        file << "=== Current Performance Statistics ===\n\n";
        file << "Total Functions Monitored: 0\n";
        file << "Total Function Calls: 0\n";
        file << "Total Execution Time: 0.000 ms\n\n";
        file << "No performance data available yet.\n";
        file.close();
    }

    mImpl->mRealTimeMonitoring.store(true);
    mImpl->mRealTimeThread = std::thread(&Impl::realTimeMonitoringLoop, mImpl.get());
}

void PerfMonitor::stopRealTimeMonitoring() {
    mImpl->stopRealTimeMonitoring();
}

std::string PerfMonitor::getRealTimeMonitoringFilePath() const {
    return mImpl->mMonitoringFilePath;
}



void PerfMonitor::reset() {
    {
        std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
        mImpl->mFunctionMetrics.clear();
    }
    {
        std::lock_guard<std::mutex> lock(mImpl->mActiveMeasurementsMutex);
        mImpl->mActiveMeasurements.clear();
    }
}

void PerfMonitor::resetFunction(const std::string& functionName) {
    std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
    auto it = mImpl->mFunctionMetrics.find(functionName);
    if (it != mImpl->mFunctionMetrics.end() && it->second) {
        it->second->reset();
    }
}

} // namespace perf