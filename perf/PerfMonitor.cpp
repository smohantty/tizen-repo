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

// PerformanceMetrics PIMPL implementation class
class PerformanceMetrics::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    // Data members with m-prefix camelCase naming
    double mMinDurationMs = std::numeric_limits<double>::max();
    double mMaxDurationMs = 0.0;
    double mTotalDurationMs = 0.0;
    double mAvgDurationMs = 0.0;
    size_t mCallCount = 0;
    mutable std::mutex mMutex; // For coordinated updates
};

// PerformanceMetrics implementation
PerformanceMetrics::PerformanceMetrics() : mImpl(new Impl()) {
}

PerformanceMetrics::~PerformanceMetrics() {
    delete mImpl;
}

void PerformanceMetrics::update(double durationMs) {
    std::lock_guard<std::mutex> lock(mImpl->mMutex);

    size_t currentCount = mImpl->mCallCount;
    if (currentCount == 0) {
        mImpl->mMinDurationMs = durationMs;
        mImpl->mMaxDurationMs = durationMs;
    } else {
        mImpl->mMinDurationMs = std::min(mImpl->mMinDurationMs, durationMs);
        mImpl->mMaxDurationMs = std::max(mImpl->mMaxDurationMs, durationMs);
    }

    // Update total and count
    mImpl->mTotalDurationMs += durationMs;
    mImpl->mCallCount++;
    size_t newCount = mImpl->mCallCount;

    // Update average
    mImpl->mAvgDurationMs = mImpl->mTotalDurationMs / newCount;
}

void PerformanceMetrics::reset() {
    std::lock_guard<std::mutex> lock(mImpl->mMutex);
    mImpl->mMinDurationMs = std::numeric_limits<double>::max();
    mImpl->mMaxDurationMs = 0.0;
    mImpl->mTotalDurationMs = 0.0;
    mImpl->mAvgDurationMs = 0.0;
    mImpl->mCallCount = 0;
}

double PerformanceMetrics::getMinDurationMs() const {
    std::lock_guard<std::mutex> lock(mImpl->mMutex);
    return mImpl->mMinDurationMs;
}

double PerformanceMetrics::getMaxDurationMs() const {
    std::lock_guard<std::mutex> lock(mImpl->mMutex);
    return mImpl->mMaxDurationMs;
}

double PerformanceMetrics::getTotalDurationMs() const {
    std::lock_guard<std::mutex> lock(mImpl->mMutex);
    return mImpl->mTotalDurationMs;
}

double PerformanceMetrics::getAvgDurationMs() const {
    std::lock_guard<std::mutex> lock(mImpl->mMutex);
    return mImpl->mAvgDurationMs;
}

size_t PerformanceMetrics::getCallCount() const {
    std::lock_guard<std::mutex> lock(mImpl->mMutex);
    return mImpl->mCallCount;
}

void PerformanceMetrics::addToGroup(size_t callCount, double totalDuration, double minDuration, double maxDuration) {
    std::lock_guard<std::mutex> lock(mImpl->mMutex);

        size_t currentCount = mImpl->mCallCount;
    size_t newCount = currentCount + callCount;

    mImpl->mCallCount = newCount;
    mImpl->mTotalDurationMs += totalDuration;

    if (currentCount == 0) {
        mImpl->mMinDurationMs = minDuration;
        mImpl->mMaxDurationMs = maxDuration;
    } else {
        mImpl->mMinDurationMs = std::min(mImpl->mMinDurationMs, minDuration);
        mImpl->mMaxDurationMs = std::max(mImpl->mMaxDurationMs, maxDuration);
    }

    if (newCount > 0) {
        mImpl->mAvgDurationMs = mImpl->mTotalDurationMs / newCount;
    }
}

PerformanceMetricsData PerformanceMetrics::getData() const {
    std::lock_guard<std::mutex> lock(mImpl->mMutex);
    PerformanceMetricsData data;
    data.mMinDurationMs = mImpl->mMinDurationMs;
    data.mMaxDurationMs = mImpl->mMaxDurationMs;
    data.mTotalDurationMs = mImpl->mTotalDurationMs;
    data.mAvgDurationMs = mImpl->mAvgDurationMs;
    data.mCallCount = mImpl->mCallCount;
    return data;
}

// PIMPL implementation class
class PerfMonitor::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    // Internal data structures with m-prefix camelCase naming
    std::unordered_map<std::string, PerformanceMetrics> mFunctionMetrics;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> mActiveMeasurements;
    std::unordered_map<std::string, std::vector<std::string>> mFunctionGroups;

    // Mutexes for thread safety
    mutable std::mutex mFunctionMetricsMutex;
    mutable std::mutex mActiveMeasurementsMutex;
    mutable std::mutex mFunctionGroupsMutex;

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
            size_t totalCalls = 0;
            double totalExecutionTime = 0.0;
            size_t functionCount = 0;

            // Create a copy of the metrics to minimize lock time
            std::unordered_map<std::string, PerformanceMetricsData> metricsCopy;
            {
                std::lock_guard<std::mutex> lock(mFunctionMetricsMutex);
                for (const auto& pair : mFunctionMetrics) {
                    metricsCopy[pair.first] = pair.second.getData();
                }
                functionCount = mFunctionMetrics.size();
            }

            for (const auto& pair : metricsCopy) {
                totalCalls += pair.second.mCallCount;
                totalExecutionTime += pair.second.mTotalDurationMs;
            }

            file << "=== Current Performance Statistics ===\n\n";
            file << "Total Functions Monitored: " << functionCount << "\n";
            file << "Total Function Calls: " << totalCalls << "\n";
            file << "Total Execution Time: " << std::fixed << std::setprecision(3)
                 << totalExecutionTime << " ms\n\n";

            file << std::left << std::setw(30) << "Function Name"
                 << std::setw(12) << "Calls"
                 << std::setw(12) << "Avg (ms)"
                 << std::setw(12) << "Min (ms)"
                 << std::setw(12) << "Max (ms)"
                 << std::setw(15) << "Total (ms)" << "\n";
            file << std::string(93, '-') << "\n";

            for (const auto& pair : metricsCopy) {
                const auto& metrics = pair.second;
                file << std::left << std::setw(30) << pair.first
                     << std::setw(12) << metrics.mCallCount
                     << std::setw(12) << std::fixed << std::setprecision(3) << metrics.mAvgDurationMs
                     << std::setw(12) << std::fixed << std::setprecision(3) << metrics.mMinDurationMs
                     << std::setw(12) << std::fixed << std::setprecision(3) << metrics.mMaxDurationMs
                     << std::setw(15) << std::fixed << std::setprecision(3) << metrics.mTotalDurationMs << "\n";
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
PerfMonitor::PerfMonitor() : mImpl(new Impl()) {
}

PerfMonitor::~PerfMonitor() {
    if (mImpl) {
        stopRealTimeMonitoring();
        delete mImpl;
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
            mImpl->mFunctionMetrics[functionName].update(duration);
        }
    }
}

void PerfMonitor::updateMetrics(const std::string& functionName, double durationMs) {
    std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
    mImpl->mFunctionMetrics[functionName].update(durationMs);
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

PerformanceMetricsData PerfMonitor::getMetrics(const std::string& functionName) const {
    std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
    auto it = mImpl->mFunctionMetrics.find(functionName);
    return (it != mImpl->mFunctionMetrics.end()) ? it->second.getData() : PerformanceMetricsData{};
}

std::vector<std::pair<std::string, PerformanceMetricsData>> PerfMonitor::getAllMetrics() const {
    std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
    std::vector<std::pair<std::string, PerformanceMetricsData>> result;
    result.reserve(mImpl->mFunctionMetrics.size());

    for (const auto& pair : mImpl->mFunctionMetrics) {
        result.emplace_back(pair.first, pair.second.getData());
    }

    return result;
}

std::string PerfMonitor::generateReport() const {
    std::ostringstream report;

    size_t totalCalls = 0;
    double totalExecutionTime = 0.0;
    size_t functionCount = 0;

    // Create a copy of the metrics to minimize lock time
    std::unordered_map<std::string, PerformanceMetricsData> metricsCopy;
    {
        std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
        for (const auto& pair : mImpl->mFunctionMetrics) {
            metricsCopy[pair.first] = pair.second.getData();
        }
        functionCount = mImpl->mFunctionMetrics.size();
    }

    for (const auto& pair : metricsCopy) {
        totalCalls += pair.second.mCallCount;
        totalExecutionTime += pair.second.mTotalDurationMs;
    }

    report << "=== Performance Monitor Report ===\n\n";
    report << "Total Functions Monitored: " << functionCount << "\n";
    report << "Total Function Calls: " << totalCalls << "\n";
    report << "Total Execution Time: " << std::fixed << std::setprecision(3)
           << totalExecutionTime << " ms\n\n";

    report << std::left << std::setw(30) << "Function Name"
           << std::setw(12) << "Calls"
           << std::setw(12) << "Avg (ms)"
           << std::setw(12) << "Min (ms)"
           << std::setw(12) << "Max (ms)"
           << std::setw(15) << "Total (ms)" << "\n";
    report << std::string(93, '-') << "\n";

    for (const auto& pair : metricsCopy) {
        const auto& metrics = pair.second;
        report << std::left << std::setw(30) << pair.first
               << std::setw(12) << metrics.mCallCount
               << std::setw(12) << std::fixed << std::setprecision(3) << metrics.mAvgDurationMs
               << std::setw(12) << std::fixed << std::setprecision(3) << metrics.mMinDurationMs
               << std::setw(12) << std::fixed << std::setprecision(3) << metrics.mMaxDurationMs
               << std::setw(15) << std::fixed << std::setprecision(3) << metrics.mTotalDurationMs << "\n";
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
    mImpl->mRealTimeThread = std::thread(&Impl::realTimeMonitoringLoop, mImpl);
}

void PerfMonitor::stopRealTimeMonitoring() {
    mImpl->stopRealTimeMonitoring();
}

std::string PerfMonitor::getRealTimeMonitoringFilePath() const {
    return mImpl->mMonitoringFilePath;
}

void PerfMonitor::addFunctionToGroup(const std::string& groupName, const std::string& functionName) {
    std::lock_guard<std::mutex> lock(mImpl->mFunctionGroupsMutex);
    mImpl->mFunctionGroups[groupName].push_back(functionName);
}

PerformanceMetricsData PerfMonitor::getGroupMetrics(const std::string& groupName) const {
    PerformanceMetrics groupMetrics;

        // Get function names from the group
    std::vector<std::string> functionNames;
    {
        std::lock_guard<std::mutex> lock(mImpl->mFunctionGroupsMutex);
        auto groupIt = mImpl->mFunctionGroups.find(groupName);
        if (groupIt == mImpl->mFunctionGroups.end()) {
            return groupMetrics.getData();
        }
        functionNames = groupIt->second;
    }

    // Access metrics with lock
    std::lock_guard<std::mutex> metricsLock(mImpl->mFunctionMetricsMutex);
    for (const auto& functionName : functionNames) {
        auto metricsIt = mImpl->mFunctionMetrics.find(functionName);
        if (metricsIt != mImpl->mFunctionMetrics.end()) {
            const auto& metrics = metricsIt->second;

            size_t callCount = metrics.getCallCount();
            double totalDuration = metrics.getTotalDurationMs();
            double minDuration = metrics.getMinDurationMs();
            double maxDuration = metrics.getMaxDurationMs();

            // Use the proper method for group aggregation
            groupMetrics.addToGroup(callCount, totalDuration, minDuration, maxDuration);
        }
    }

    return groupMetrics.getData();
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
    {
        std::lock_guard<std::mutex> lock(mImpl->mFunctionGroupsMutex);
        mImpl->mFunctionGroups.clear();
    }
}

void PerfMonitor::resetFunction(const std::string& functionName) {
    std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
    auto it = mImpl->mFunctionMetrics.find(functionName);
    if (it != mImpl->mFunctionMetrics.end()) {
        it->second.reset();
    }
}

} // namespace perf