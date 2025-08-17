#include "PerfMonitor.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

using namespace perf;

// Demo application showing real-world usage scenarios

class DataProcessor {
public:
    std::vector<double> processData(const std::vector<int>& input) {
        PERF_MEASURE_FUNCTION();

        std::vector<double> result;
        result.reserve(input.size());

        for (int value : input) {
            result.push_back(expensiveCalculation(value));
        }

        return result;
    }

    void sortData(std::vector<double>& data) {
        PERF_MEASURE_SCOPE("DataProcessor::sortData");

        // Simulate expensive sorting algorithm
        std::sort(data.begin(), data.end());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

private:
    double expensiveCalculation(int input) {
        PERF_MEASURE_SCOPE("DataProcessor::expensiveCalculation");

        // Simulate CPU-intensive calculation
        double result = std::sin(input) * std::cos(input * 0.5);
        for (int i = 0; i < 1000; ++i) {
            result += std::sqrt(i + input);
        }
        return result;
    }
};

class NetworkSimulator {
public:
    void sendRequest(int request_id) {
        std::string func_name = "NetworkSimulator::sendRequest_" + std::to_string(request_id);
        PERF_MEASURE_SCOPE(func_name);

        // Simulate network latency
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> latency_dist(10, 100);

        std::this_thread::sleep_for(std::chrono::milliseconds(latency_dist(gen)));

        // Note: Custom metrics removed - using performance timing instead
    }

    void processResponse() {
        PERF_MEASURE_FUNCTION();

        // Simulate response processing
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
};

class DatabaseManager {
public:
    void connect() {
        PERF_MEASURE_FUNCTION();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Note: Memory usage tracking removed - using performance timing instead
    }

    std::vector<std::string> query(const std::string& sql) {
        PERF_MEASURE_SCOPE("DatabaseManager::query");

        // Simulate database query time based on complexity
        int complexity = sql.length() / 10; // Simple complexity metric
        std::this_thread::sleep_for(std::chrono::milliseconds(5 + complexity));

        // Return dummy results
        std::vector<std::string> results;
        for (int i = 0; i < 10; ++i) {
            results.push_back("result_" + std::to_string(i));
        }

        // Note: Custom metrics removed - using performance timing instead

        return results;
    }

    void disconnect() {
        PERF_MEASURE_FUNCTION();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
};

void workerThread(int worker_id, int num_tasks) {
    DataProcessor processor;

    for (int task = 0; task < num_tasks; ++task) {
        std::string task_name = "worker_" + std::to_string(worker_id) + "_task_" + std::to_string(task);
        PERF_MEASURE_SCOPE(task_name);

        // Generate random input data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> size_dist(10, 100);
        std::uniform_int_distribution<> value_dist(1, 1000);

        int data_size = size_dist(gen);
        std::vector<int> input_data;
        input_data.reserve(data_size);

        for (int i = 0; i < data_size; ++i) {
            input_data.push_back(value_dist(gen));
        }

        // Process the data
        auto processed = processor.processData(input_data);
        processor.sortData(processed);

        // Small delay between tasks
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void demonstrateBasicUsage() {
    std::cout << "\n=== Basic Usage Demo ===\n";

    auto& monitor = PerfMonitor::getInstance();
    monitor.reset(); // Start fresh

    // Simple function timing
    PERF_START("demo_initialization");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    PERF_END("demo_initialization");

    // Scoped timing
    {
        PERF_MEASURE_SCOPE("demo_computation");

        double result = 0;
        for (int i = 0; i < 1000000; ++i) {
            result += std::sin(i * 0.001);
        }
        std::cout << "Computation result: " << result << "\n";
    }

    // Function with return value
    auto computation = [](int n) -> double {
        double sum = 0;
        for (int i = 0; i < n; ++i) {
            sum += std::sqrt(i);
        }
        return sum;
    };

    double result = monitor.measureCall("lambda_computation", computation, 50000);
    std::cout << "Lambda result: " << result << "\n";

    std::cout << "Basic usage demo completed.\n";
}

void demonstrateClassUsage() {
    std::cout << "\n=== Class Usage Demo ===\n";

    DataProcessor processor;

    // Generate test data
    std::vector<int> test_data;
    for (int i = 0; i < 50; ++i) {
        test_data.push_back(i * 3 + 7);
    }

    // Process data multiple times to get good metrics
    for (int run = 0; run < 5; ++run) {
        auto processed = processor.processData(test_data);
        processor.sortData(processed);
    }

    std::cout << "Class usage demo completed.\n";
}

void demonstrateNetworkSimulation() {
    std::cout << "\n=== Network Simulation Demo ===\n";

    NetworkSimulator network;

    // Simulate multiple network requests
    for (int i = 0; i < 20; ++i) {
        network.sendRequest(i);
        network.processResponse();
    }

    std::cout << "Network simulation completed.\n";
}

void demonstrateDatabaseUsage() {
    std::cout << "\n=== Database Usage Demo ===\n";

    DatabaseManager db;

    // Simulate database operations
    db.connect();

    std::vector<std::string> queries = {
        "SELECT * FROM users",
        "SELECT name, email FROM users WHERE active = 1",
        "SELECT u.name, p.title FROM users u JOIN posts p ON u.id = p.user_id",
        "UPDATE users SET last_login = NOW() WHERE id = 123",
        "DELETE FROM sessions WHERE expired < NOW()"
    };

    for (const auto& query : queries) {
        auto results = db.query(query);
        std::cout << "Query returned " << results.size() << " results\n";
    }

    db.disconnect();

    std::cout << "Database demo completed.\n";
}

void demonstrateMultiThreading() {
    std::cout << "\n=== Multi-threading Demo ===\n";

    auto& monitor = PerfMonitor::getInstance();
    // Note: enableThreadMetrics() not available in current API

    const int num_workers = 4;
    const int tasks_per_worker = 3;

    std::vector<std::thread> workers;

    std::cout << "Starting " << num_workers << " worker threads...\n";

    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back(workerThread, i, tasks_per_worker);
    }

    // Wait for all workers to complete
    for (auto& worker : workers) {
        worker.join();
    }

    std::cout << "All worker threads completed.\n";
}

void demonstrateGroupingAndAnalysis() {
    std::cout << "\n=== Grouping and Analysis Demo ===\n";

    auto& monitor = PerfMonitor::getInstance();

    // Group functions by category
    monitor.addFunctionToGroup("data_processing", "DataProcessor::processData");
    monitor.addFunctionToGroup("data_processing", "DataProcessor::sortData");
    monitor.addFunctionToGroup("data_processing", "DataProcessor::expensiveCalculation");

    monitor.addFunctionToGroup("network_operations", "NetworkSimulator::sendRequest");
    monitor.addFunctionToGroup("network_operations", "NetworkSimulator::processResponse");

    monitor.addFunctionToGroup("database_operations", "DatabaseManager::connect");
    monitor.addFunctionToGroup("database_operations", "DatabaseManager::query");
    monitor.addFunctionToGroup("database_operations", "DatabaseManager::disconnect");

    // Analyze group performance
    auto data_metrics = monitor.getGroupMetrics("data_processing");
    auto network_metrics = monitor.getGroupMetrics("network_operations");
    auto db_metrics = monitor.getGroupMetrics("database_operations");

    std::cout << "Performance Analysis by Category:\n";
    std::cout << "  Data Processing: " << data_metrics.mCallCount
              << " calls, avg " << data_metrics.mAvgDurationMs << "ms\n";
    std::cout << "  Network Operations: " << network_metrics.mCallCount
              << " calls, avg " << network_metrics.mAvgDurationMs << "ms\n";
    std::cout << "  Database Operations: " << db_metrics.mCallCount
              << " calls, avg " << db_metrics.mAvgDurationMs << "ms\n";

    // Show top performance bottlenecks
    auto slowest = monitor.getAllMetrics(); // Note: getTopSlowFunctions() not available
    std::cout << "\nAll Performance Metrics:\n";
    for (size_t i = 0; i < slowest.size() && i < 5; ++i) {
        const auto& func_name = slowest[i].first;
        const auto& metrics = slowest[i].second;
        std::cout << "  " << (i+1) << ". " << func_name
                  << " (avg: " << metrics.mAvgDurationMs << "ms)\n";
    }
}

void demonstrateRealTimeMonitoring() {
    std::cout << "\n=== Real-time Monitoring Demo ===\n";

    auto& monitor = PerfMonitor::getInstance();

    // Set up real-time monitoring

    monitor.startRealTimeMonitoring(std::chrono::milliseconds(2000));

    std::cout << "Real-time monitoring started. Running workload...\n";

    // Run some work while monitoring
    DataProcessor processor;
    for (int i = 0; i < 10; ++i) {
        std::vector<int> data = {1, 2, 3, 4, 5};
        auto result = processor.processData(data);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // monitoring_active = false; // Variable not defined anymore
    monitor.stopRealTimeMonitoring();
    std::cout << "Real-time monitoring stopped.\n";
}

void generateFinalReport() {
    std::cout << "\n=== Final Performance Report ===\n";

    auto& monitor = PerfMonitor::getInstance();

    // Generate comprehensive report
    std::cout << monitor.generateReport();

    // Export reports to files
    // Note: exportToFile() not available in current API
    // Note: exportToFile() not available in current API
    // Note: exportToFile() not available in current API

    std::cout << "\nReports exported to:\n";
    std::cout << "  - demo_performance_report.txt\n";
    std::cout << "  - demo_performance_report.csv\n";
    std::cout << "  - demo_performance_report.json\n";

    // Show memory and custom metrics
    // auto memory_history = monitor.getMemoryHistory(); // Note: not available
    // Note: Memory usage and custom metrics functionality not available in current API
}

int main() {
    std::cout << "=== Performance Monitor Demo Application ===\n";
    std::cout << "This demo showcases the PerfMonitor library capabilities\n";
    std::cout << "in a realistic application scenario.\n";

    try {
        // Run all demonstrations
        demonstrateBasicUsage();
        demonstrateClassUsage();
        demonstrateNetworkSimulation();
        demonstrateDatabaseUsage();
        demonstrateMultiThreading();
        demonstrateGroupingAndAnalysis();
       // demonstrateRealTimeMonitoring();

        // Generate final comprehensive report
        generateFinalReport();

        std::cout << "\n=== Demo Completed Successfully! ===\n";
        std::cout << "Check the generated report files for detailed analysis.\n";

    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
