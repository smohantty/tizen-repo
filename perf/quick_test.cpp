#include "PerfMonitor.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace perf;

void testFunction() {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

int computeSum(int n) {
    int sum = 0;
    for (int i = 0; i < n; ++i) {
        sum += i;
    }
    return sum;
}

int main() {
    std::cout << "=== Quick PerfMonitor Test ===\n";

    auto& monitor = PerfMonitor::getInstance();
    monitor.reset();

    // Test 1: Scoped timing
    {
        PERF_MEASURE_SCOPE("scoped_test");
        testFunction();
    }

    // Test 2: Manual timing
    PERF_START("manual_test");
    testFunction();
    PERF_END("manual_test");

    // Test 3: Function with return value
    int result = monitor.measureCall("compute_test", computeSum, 1000);
    std::cout << "Computation result: " << result << "\n";

    // Test 4: Multiple calls
    for (int i = 0; i < 5; ++i) {
        PERF_MEASURE_SCOPE("repeated_test");
        testFunction();
    }

    // Display results
    std::cout << "\n=== Results ===\n";

    auto metrics = monitor.getMetrics("scoped_test");
    std::cout << "scoped_test: " << metrics.mCallCount << " calls, "
              << metrics.mAvgDurationMs << "ms avg\n";

    metrics = monitor.getMetrics("repeated_test");
    std::cout << "repeated_test: " << metrics.mCallCount << " calls, "
              << metrics.mAvgDurationMs << "ms avg\n";

    std::cout << "\nTotal functions monitored: " << monitor.getAllMetrics().size() << "\n";

    // Generate simple report
    std::cout << "\n=== Performance Report ===\n";
    std::cout << monitor.generateReport();

    std::cout << "\n=== Test Completed Successfully! ===\n";
    return 0;
}
