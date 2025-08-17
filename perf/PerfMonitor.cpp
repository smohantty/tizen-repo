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

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace perf {

void PerformanceMetrics::update(double durationMs) {
    std::lock_guard<std::mutex> lock(mMutex);

    size_t currentCount = mCallCount.load();
    if (currentCount == 0) {
        mMinDurationMs = durationMs;
        mMaxDurationMs = durationMs;
    } else {
        mMinDurationMs = std::min(mMinDurationMs, durationMs);
        mMaxDurationMs = std::max(mMaxDurationMs, durationMs);
    }

    // Update total and count
    mTotalDurationMs += durationMs;
    size_t newCount = mCallCount.fetch_add(1) + 1;

    // Update average
    mAvgDurationMs = mTotalDurationMs / newCount;
}

void PerformanceMetrics::reset() {
    std::lock_guard<std::mutex> lock(mMutex);
    mMinDurationMs = std::numeric_limits<double>::max();
    mMaxDurationMs = 0.0;
    mTotalDurationMs = 0.0;
    mAvgDurationMs = 0.0;
    mCallCount.store(0);
}

PerformanceMetrics::PerformanceMetrics(const PerformanceMetrics& other) {
    std::lock_guard<std::mutex> lock(other.mMutex);
    mMinDurationMs = other.mMinDurationMs;
    mMaxDurationMs = other.mMaxDurationMs;
    mTotalDurationMs = other.mTotalDurationMs;
    mAvgDurationMs = other.mAvgDurationMs;
    mCallCount.store(other.mCallCount.load());
}

PerformanceMetrics& PerformanceMetrics::operator=(const PerformanceMetrics& other) {
    if (this != &other) {
        std::lock(mMutex, other.mMutex);
        std::lock_guard<std::mutex> lock1(mMutex, std::adopt_lock);
        std::lock_guard<std::mutex> lock2(other.mMutex, std::adopt_lock);

        mMinDurationMs = other.mMinDurationMs;
        mMaxDurationMs = other.mMaxDurationMs;
        mTotalDurationMs = other.mTotalDurationMs;
        mAvgDurationMs = other.mAvgDurationMs;
        mCallCount.store(other.mCallCount.load());
    }
    return *this;
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
            std::unordered_map<std::string, PerformanceMetrics> metricsCopy;
            {
                std::lock_guard<std::mutex> lock(mFunctionMetricsMutex);
                metricsCopy = mFunctionMetrics;
                functionCount = mFunctionMetrics.size();
            }

            for (const auto& pair : metricsCopy) {
                totalCalls += pair.second.mCallCount.load();
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
                     << std::setw(12) << metrics.mCallCount.load()
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
    std::string tempDir;

#ifdef _WIN32
    const char* tempEnv = std::getenv("TEMP");
    if (tempEnv) {
        tempDir = tempEnv;
    } else {
        tempDir = "C:\\temp";
    }
    return tempDir + "\\perfmonitor_" + std::to_string(GetCurrentProcessId()) + ".log";
#else
    const char* tempEnv = std::getenv("TMPDIR");
    if (tempEnv) {
        tempDir = tempEnv;
    } else {
        tempDir = "/tmp";
    }
    return tempDir + "/perfmonitor_" + std::to_string(getpid()) + ".log";
#endif
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

PerformanceMetrics PerfMonitor::getMetrics(const std::string& functionName) const {
    std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
    auto it = mImpl->mFunctionMetrics.find(functionName);
    return (it != mImpl->mFunctionMetrics.end()) ? it->second : PerformanceMetrics{};
}

std::vector<std::pair<std::string, PerformanceMetrics>> PerfMonitor::getAllMetrics() const {
    std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
    std::vector<std::pair<std::string, PerformanceMetrics>> result;
    result.reserve(mImpl->mFunctionMetrics.size());

    for (const auto& pair : mImpl->mFunctionMetrics) {
        result.emplace_back(pair);
    }

    return result;
}

std::string PerfMonitor::generateReport() const {
    std::ostringstream report;

    size_t totalCalls = 0;
    double totalExecutionTime = 0.0;
    size_t functionCount = 0;

    // Create a copy of the metrics to minimize lock time
    std::unordered_map<std::string, PerformanceMetrics> metricsCopy;
    {
        std::lock_guard<std::mutex> lock(mImpl->mFunctionMetricsMutex);
        metricsCopy = mImpl->mFunctionMetrics;
        functionCount = mImpl->mFunctionMetrics.size();
    }

    for (const auto& pair : metricsCopy) {
        totalCalls += pair.second.mCallCount.load();
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
               << std::setw(12) << metrics.mCallCount.load()
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

PerformanceMetrics PerfMonitor::getGroupMetrics(const std::string& groupName) const {
    PerformanceMetrics groupMetrics;

        // Get function names from the group
    std::vector<std::string> functionNames;
    {
        std::lock_guard<std::mutex> lock(mImpl->mFunctionGroupsMutex);
        auto groupIt = mImpl->mFunctionGroups.find(groupName);
        if (groupIt == mImpl->mFunctionGroups.end()) {
            return groupMetrics;
        }
        functionNames = groupIt->second;
    }

    // Access metrics with lock
    std::lock_guard<std::mutex> metricsLock(mImpl->mFunctionMetricsMutex);
    for (const auto& functionName : functionNames) {
        auto metricsIt = mImpl->mFunctionMetrics.find(functionName);
        if (metricsIt != mImpl->mFunctionMetrics.end()) {
            const auto& metrics = metricsIt->second;
            std::lock_guard<std::mutex> metricsLockLocal(metrics.mMutex);

            size_t callCount = metrics.mCallCount.load();
            double totalDuration = metrics.mTotalDurationMs;
            double minDuration = metrics.mMinDurationMs;
            double maxDuration = metrics.mMaxDurationMs;

            size_t currentGroupCount = groupMetrics.mCallCount.load();
            groupMetrics.mCallCount.store(currentGroupCount + callCount);
            groupMetrics.mTotalDurationMs += totalDuration;

            if (currentGroupCount == 0) {
                groupMetrics.mMinDurationMs = minDuration;
                groupMetrics.mMaxDurationMs = maxDuration;
            } else {
                groupMetrics.mMinDurationMs = std::min(groupMetrics.mMinDurationMs, minDuration);
                groupMetrics.mMaxDurationMs = std::max(groupMetrics.mMaxDurationMs, maxDuration);
            }
        }
    }

    size_t totalCalls = groupMetrics.mCallCount.load();
    if (totalCalls > 0) {
        groupMetrics.mAvgDurationMs = groupMetrics.mTotalDurationMs / totalCalls;
    }

    return groupMetrics;
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