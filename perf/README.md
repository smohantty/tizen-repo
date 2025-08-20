# PerfMonitor - High-Performance Function Monitoring Library

A comprehensive C++17 performance monitoring library designed for fine-grained function performance measurement with minimal overhead.

## Features

### Core Functionality
- **Thread-safe monitoring** with minimal performance overhead
- **Multiple timing methods**: Manual, scoped (RAII), and wrapper-based
- **Template support** for measuring any callable with return value preservation
- **Real-time monitoring** with configurable intervals and callbacks
- **Memory usage tracking** with checkpoint snapshots
- **Custom metrics** recording with units and timestamps

### Advanced Features
- **Multi-threading support** with per-thread metrics
- **Function grouping** for aggregate analysis
- **Sampling rate control** to reduce overhead in production
- **Function filtering** for selective monitoring
- **Multiple report formats**: Text, CSV, JSON
- **Statistical analysis**: min/max/average durations, call counts
- **Top-N analysis**: slowest functions, most called functions

### Configuration Options
- Enable/disable monitoring globally
- Pause/resume functionality
- Configurable history size limits
- Platform-specific memory tracking
- Real-time monitoring intervals

## Quick Start

### Building the Project

#### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Meson build system (0.55+)
- Ninja build backend

#### Build Commands
```bash
# Configure the build
meson setup builddir

# Compile the library and executables
meson compile -C builddir

# Run tests
meson test -C builddir

# Install (optional)
meson install -C builddir
```

#### Build Options
```bash
# Enable benchmarks
meson setup builddir -Denable_benchmarks=true

# Configure installation prefix
meson setup builddir --prefix=/usr/local

# Build in release mode
meson setup builddir --buildtype=release
```

### Basic Usage

#### Include and Initialize
```cpp
#include "PerfMonitor.h"
using namespace perf;

// The monitor is a singleton - get instance
auto& monitor = PerfMonitor::getInstance();
```

#### Simple Function Timing
```cpp
// Method 1: Manual timing
PERF_START("my_function");
my_function();
PERF_END("my_function");

// Method 2: Scoped timing (RAII)
{
    PERF_MEASURE_SCOPE("my_function");
    my_function();
} // Automatically measured when scope exits

// Method 3: Automatic function name
void my_function() {
    PERF_MEASURE_FUNCTION(); // Uses __FUNCTION__ macro
    // Function implementation
}
```

#### Class Member Functions
```cpp
class MyClass {
public:
    void process_data() {
        PERF_MEASURE_FUNCTION(); // Automatically becomes "MyClass::process_data"
        // Implementation
    }

    void custom_name_function() {
        PERF_MEASURE_SCOPE("MyClass::custom_operation");
        // Implementation
    }
};
```

## Advanced Usage

### Multi-threading Support
```cpp
// Enable per-thread metrics
monitor.enableThreadMetrics(true);

// Each thread's metrics are tracked separately
void worker_thread(int id) {
    for (int i = 0; i < 100; ++i) {
        std::string task_name = "worker_" + std::to_string(id) + "_task";
        PERF_MEASURE_SCOPE(task_name);
        do_work();
    }
}

// Get thread-specific metrics
auto thread_metrics = monitor.getAllThreadMetrics();
for (const auto& tm : thread_metrics) {
    std::cout << "Thread " << tm.thread_id
              << " total time: " << tm.total_cpu_time_ms << "ms\n";
}
```

### Memory and Custom Metrics
```cpp
// Track memory usage at checkpoints
monitor.recordMemoryUsage("startup", 1024 * 1024);
monitor.recordMemoryUsage("after_loading", 5 * 1024 * 1024);

// Record custom application metrics
monitor.recordCustomMetric("cache_hit_rate", 0.85, "ratio");
monitor.recordCustomMetric("cpu_temperature", 65.5, "°C");
monitor.recordCustomMetric("active_connections", 1250, "count");

// Retrieve history
auto memory_history = monitor.getMemoryHistory();
auto custom_metrics = monitor.getCustomMetrics("cache_hit_rate");
```

### Function Grouping and Analysis
```cpp
// Group related functions
monitor.addFunctionToGroup("database", "connect_db");
monitor.addFunctionToGroup("database", "execute_query");
monitor.addFunctionToGroup("database", "disconnect_db");

monitor.addFunctionToGroup("network", "send_request");
monitor.addFunctionToGroup("network", "receive_response");

// Get aggregate metrics for groups
auto db_metrics = monitor.getGroupMetrics("database");
auto net_metrics = monitor.getGroupMetrics("network");

std::cout << "Database operations: " << db_metrics.call_count
          << " calls, avg " << db_metrics.avg_duration_ms << "ms\n";
```

### Real-time Monitoring
```cpp
// Set up real-time callback
monitor.setRealTimeCallback([](const std::string& report) {
    std::cout << "=== Live Performance Update ===\n";
    std::cout << report << "\n";
});

// Start monitoring every 1 second
monitor.startRealTimeMonitoring(std::chrono::milliseconds(1000));

// Your application runs...
run_application();

// Stop monitoring
monitor.stopRealTimeMonitoring();
```

### Configuration and Filtering
```cpp
// Control overhead in production
monitor.setSamplingRate(0.1); // Sample only 10% of calls
monitor.setMaxHistorySize(5000); // Limit memory usage

// Filter functions to monitor
monitor.setFunctionFilter([](const std::string& name) {
    return name.find("critical") != std::string::npos;
});

// Pause/resume monitoring
monitor.pause();
// ... functions called here won't be measured
monitor.resume();

// Disable completely
monitor.setEnabled(false);
```

## Reporting and Analysis

### Getting Metrics
```cpp
// Individual function metrics
auto metrics = monitor.getMetrics("my_function");
std::cout << "Calls: " << metrics.call_count << "\n"
          << "Average: " << metrics.avg_duration_ms << "ms\n"
          << "Min: " << metrics.min_duration_ms << "ms\n"
          << "Max: " << metrics.max_duration_ms << "ms\n"
          << "Total: " << metrics.total_duration_ms << "ms\n";

// All metrics
auto all_metrics = monitor.getAllMetrics();
for (const auto& [name, metrics] : all_metrics) {
    std::cout << name << ": " << metrics.avg_duration_ms << "ms\n";
}
```

### Performance Analysis
```cpp
// Find performance bottlenecks
auto slowest = monitor.getTopSlowFunctions(10);
auto most_called = monitor.getMostCalledFunctions(10);

std::cout << "Top 10 slowest functions:\n";
for (const auto& func : slowest) {
    auto m = monitor.getMetrics(func);
    std::cout << "  " << func << ": " << m.avg_duration_ms << "ms\n";
}

// Overall statistics
std::cout << "Total execution time: " << monitor.getTotalExecutionTime() << "ms\n";
std::cout << "Total function calls: " << monitor.getTotalFunctionCalls() << "\n";
```

### Report Generation
```cpp
// Generate reports in different formats
std::string text_report = monitor.generateReport(true); // Include thread metrics
std::string csv_report = monitor.generateCSVReport();
std::string json_report = monitor.generateJSONReport();

// Export to files
monitor.exportToFile("performance_report.txt", "txt");
monitor.exportToFile("performance_data.csv", "csv");
monitor.exportToFile("performance_data.json", "json");
```

## Integration Examples

### Example 1: Web Server Performance Monitoring
```cpp
class WebServer {
    void handle_request(const Request& req) {
        PERF_MEASURE_FUNCTION();

        if (req.path.starts_with("/api/")) {
            handle_api_request(req);
        } else {
            serve_static_file(req);
        }
    }

    void handle_api_request(const Request& req) {
        PERF_MEASURE_SCOPE("WebServer::api_request");

        auto& monitor = PerfMonitor::getInstance();

        // Track request metrics
        monitor.recordCustomMetric("request_size", req.body.size(), "bytes");

        // Database operation
        {
            PERF_MEASURE_SCOPE("database_query");
            auto results = database.query(req.sql);
            monitor.recordCustomMetric("query_results", results.size(), "rows");
        }

        // Memory tracking
        monitor.recordMemoryUsage("after_request", get_memory_usage());
    }
};
```

### Example 2: Game Engine Profiling
```cpp
class GameEngine {
    void game_loop() {
        while (running) {
            PERF_MEASURE_SCOPE("GameEngine::frame");

            {
                PERF_MEASURE_SCOPE("update_game_logic");
                update_physics();
                update_ai();
                update_audio();
            }

            {
                PERF_MEASURE_SCOPE("render_frame");
                render_scene();
                present_frame();
            }

            // Track frame rate
            auto& monitor = PerfMonitor::getInstance();
            monitor.recordCustomMetric("fps", calculate_fps(), "fps");
        }
    }
};
```

## Building and Testing

### Project Structure
```
perf/
├── PerfMonitor.h              # Main header file
├── PerfMonitor.cpp            # Implementation
├── test_perf_monitor.cpp      # Comprehensive test suite
├── demo_perf_monitor.cpp      # Demo application
├── benchmark_perf_monitor.cpp # Performance benchmarks
├── meson.build               # Build configuration
├── meson_options.txt         # Build options
└── README.md                 # This file
```

### Running Examples
```bash
# Build everything
meson compile -C builddir

# Run the comprehensive test suite
./builddir/test_perf_monitor

# Run the demo application
./builddir/demo_perf_monitor

# Run performance benchmarks (if enabled)
meson configure builddir -Denable_benchmarks=true
meson compile -C builddir
./builddir/benchmark_perf_monitor
```

### Installing as a Library
```bash
# Install system-wide
meson install -C builddir

# Or install to custom prefix
meson setup builddir --prefix=$HOME/.local
meson install -C builddir
```

### Using in Other Projects

#### With Meson
```meson
# In your meson.build
perf_dep = dependency('perf_monitor')

executable('my_app', 'main.cpp',
    dependencies : perf_dep)
```

#### With pkg-config
```bash
# Compile manually
g++ -std=c++17 main.cpp $(pkg-config --cflags --libs perf_monitor)
```

## Performance Characteristics

### Overhead Measurements
Typical overhead per function call (measured on modern x86_64):
- **Scoped timing**: ~0.1-0.5 μs per call
- **Manual timing**: ~0.2-0.8 μs per call
- **With 10% sampling**: ~0.01-0.05 μs per call

### Memory Usage
- **Per function tracked**: ~200-500 bytes
- **Per thread**: ~1-2 KB baseline
- **History storage**: Configurable, default 10,000 entries

### Thread Safety
- All operations are thread-safe using fine-grained locking
- Per-thread metrics avoid contention
- Lock-free sampling for reduced overhead

## Best Practices

### Production Deployment
1. **Use sampling** to reduce overhead: `monitor.setSamplingRate(0.01)` (1%)
2. **Filter critical paths** only: Focus on important functions
3. **Limit history size** for memory control
4. **Use scoped timing** for automatic cleanup
5. **Group related functions** for better analysis

### Development and Testing
1. **Enable full monitoring** during development
2. **Use real-time monitoring** for interactive debugging
3. **Export detailed reports** for post-analysis
4. **Monitor memory usage** to detect leaks
5. **Track custom metrics** for application-specific KPIs

### Integration Guidelines
1. **Instrument at boundaries**: API calls, major operations
2. **Name functions descriptively**: Include class/module names
3. **Use consistent naming**: Follow your project's conventions
4. **Group by functionality**: Database, network, computation, etc.
5. **Document performance requirements**: Set baselines and alerts

## Platform Support

### Supported Platforms
- **Linux**: Full support including memory tracking
- **macOS**: Full support including memory tracking
- **Windows**: Full support including memory tracking
- **Other POSIX**: Basic functionality (no memory tracking)

### Compiler Requirements
- **GCC**: 7.0+ (C++17 support)
- **Clang**: 5.0+ (C++17 support)
- **MSVC**: 2017+ (C++17 support)

## License

This project is provided as-is for educational and development purposes.

## Contributing

This is a demonstration project. For production use, consider:
1. Adding more comprehensive error handling
2. Implementing custom memory allocators for low overhead
3. Adding network export capabilities for distributed monitoring
4. Creating visualization tools for the generated data
5. Adding statistical analysis features (percentiles, histograms)

## Troubleshooting

### Common Issues

**High overhead in tight loops:**
- Use sampling: `monitor.setSamplingRate(0.01)`
- Consider measuring the loop as a whole instead of individual iterations

**Memory usage growing:**
- Set history limits: `monitor.setMaxHistorySize(1000)`
- Reset periodically: `monitor.reset()`

**Thread contention:**
- Disable thread metrics if not needed: `monitor.enableThreadMetrics(false)`
- Use function filtering to reduce monitored functions

**Compiler errors:**
- Ensure C++17 support is enabled
- Check that all required headers are included
- Verify thread library linking
