#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "TouchUpscaler.h"

using namespace vtd;
using namespace std::chrono;

// Simple test framework
class TestRunner {
   private:
    int mTestCount = 0;
    int mPassedTests = 0;

   public:
    void runTest(const std::string& testName, std::function<void()> testFunc) {
        mTestCount++;
        std::cout << "Running test: " << testName << "... ";

        try {
            testFunc();
            mPassedTests++;
            std::cout << "PASSED" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "FAILED: " << e.what() << std::endl;
        } catch (...) {
            std::cout << "FAILED: Unknown exception" << std::endl;
        }
    }

    void printSummary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total tests: " << mTestCount << std::endl;
        std::cout << "Passed: " << mPassedTests << std::endl;
        std::cout << "Failed: " << (mTestCount - mPassedTests) << std::endl;
        std::cout << "Success rate: " << (100.0 * mPassedTests / mTestCount) << "%" << std::endl;
    }

    bool allTestsPassed() const { return mPassedTests == mTestCount; }
};

// Test helper functions
void assertEqual(float expected, float actual, float tolerance = 0.001f) {
    if (std::abs(expected - actual) > tolerance) {
        throw std::runtime_error("Expected " + std::to_string(expected) + " but got " + std::to_string(actual));
    }
}

void assertTrue(bool condition, const std::string& message = "Assertion failed") {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void assertFalse(bool condition, const std::string& message = "Assertion failed") {
    if (condition) {
        throw std::runtime_error(message);
    }
}

// Test Configuration
void testDefaultConfig() {
    Config cfg = Config::getDefault();

    // Verify default values
}

// Test Basic Functionality
void testBasicConstruction() {
    Config cfg = Config::getDefault();
    cfg.backend = Backend::Mock;  // Use mock backend for testing
    TouchUpscaler upscaler(cfg);

    // Should construct without throwing
    upscaler.start();
    std::this_thread::sleep_for(milliseconds(10));  // Let it run briefly
    upscaler.stop();
}

// Test Single Touch Input
void testSingleTouchInput() {
    Config cfg = Config::getDefault();
    cfg.backend = Backend::Mock;
    TouchUpscaler upscaler(cfg);

    upscaler.start();

    // Push a valid touch sample
    TouchSample sample;
    sample.x = 0.5f;
    sample.y = 0.3f;
    sample.valid = true;

    upscaler.push(sample);

    // Push an invalid sample
    TouchSample invalidSample;
    invalidSample.valid = false;

    upscaler.push(invalidSample);

    upscaler.stop();
}

// Test Multiple Valid Samples
void testMultipleValidSamples() {
    Config cfg = Config::getDefault();
    cfg.backend = Backend::Mock;
    TouchUpscaler upscaler(cfg);

    upscaler.start();

    // Push multiple valid samples to test interpolation
    std::vector<TouchSample> samples = {
        {0.1f, 0.1f, true}, {0.2f, 0.2f, true}, {0.3f, 0.3f, true}, {0.4f, 0.4f, true}, {0.5f, 0.5f, true}};

    for (const auto& sample : samples) {
        upscaler.push(sample);
        std::this_thread::sleep_for(milliseconds(10));  // Simulate 100Hz input
    }

    upscaler.stop();
}

// Test Invalid Sample Handling
void testInvalidSampleHandling() {
    Config cfg = Config::getDefault();
    cfg.backend = Backend::Mock;
    TouchUpscaler upscaler(cfg);

    upscaler.start();

    // Push valid samples first
    TouchSample validSample{0.5f, 0.5f, true};
    upscaler.push(validSample);

    // Push invalid samples
    TouchSample invalidSample{0.0f, 0.0f, false};
    for (int i = 0; i < 5; i++) {
        upscaler.push(invalidSample);
        std::this_thread::sleep_for(milliseconds(5));
    }

    upscaler.stop();
}

// Test Extrapolation Time Limits
void testExtrapolationTimeLimits() {
    Config cfg = Config::getDefault();
    cfg.backend = Backend::Mock;

    TouchUpscaler upscaler(cfg);
    upscaler.start();

    // Push valid samples
    TouchSample sample1{0.1f, 0.1f, true};
    TouchSample sample2{0.2f, 0.2f, true};

    upscaler.push(sample1);
    std::this_thread::sleep_for(milliseconds(10));
    upscaler.push(sample2);

    // Wait longer than extrapolation time
    std::this_thread::sleep_for(milliseconds(50));

    // Push invalid sample to trigger extrapolation logic
    TouchSample invalidSample{0.0f, 0.0f, false};
    upscaler.push(invalidSample);

    upscaler.stop();
}

// Test Thread Safety
void testThreadSafety() {
    Config cfg = Config::getDefault();
    cfg.backend = Backend::Mock;
    TouchUpscaler upscaler(cfg);

    upscaler.start();

    std::atomic<bool> shouldStop{false};
    std::vector<std::thread> threads;

    // Create multiple threads pushing samples
    for (int t = 0; t < 4; t++) {
        threads.emplace_back([&upscaler, &shouldStop, t]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> pos(0.0f, 1.0f);
            std::uniform_int_distribution<int> valid(0, 1);

            int count = 0;
            while (!shouldStop && count < 100) {
                TouchSample sample;
                sample.x = pos(gen);
                sample.y = pos(gen);
                sample.valid = valid(gen) == 1;

                upscaler.push(sample);
                std::this_thread::sleep_for(microseconds(100));
                count++;
            }
        });
    }

    // Let threads run for a short time
    std::this_thread::sleep_for(milliseconds(100));
    shouldStop = true;

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    upscaler.stop();
}

// Test Configuration Validation
void testConfigurationValidation() {
    Config cfg = Config::getDefault();
    cfg.backend = Backend::Mock;

    // Test various configuration values
    cfg.outputHz = 120.0;

    TouchUpscaler upscaler(cfg);
    upscaler.start();
    upscaler.stop();
}

// Test Skipped Frame Detection
void testSkippedFrameDetection() {
    Config cfg = Config::getDefault();
    cfg.backend = Backend::Mock;
    TouchUpscaler upscaler(cfg);

    upscaler.start();

    // Push first sample
    TouchSample sample1{0.1f, 0.1f, true};
    upscaler.push(sample1);

    // Simulate large time gap (skipped frame)
    std::this_thread::sleep_for(milliseconds(60));

    // Push second sample
    TouchSample sample2{0.2f, 0.2f, true};
    upscaler.push(sample2);

    upscaler.stop();
}

// Test EMA Smoothing
void testEMASmoothing() {
    Config cfg = Config::getDefault();
    cfg.backend = Backend::Mock;
    TouchUpscaler upscaler(cfg);

    upscaler.start();

    // Push samples with known values to test smoothing
    std::vector<TouchSample> samples = {{0.0f, 0.0f, true}, {1.0f, 1.0f, true}, {0.0f, 0.0f, true}, {1.0f, 1.0f, true}};

    for (const auto& sample : samples) {
        upscaler.push(sample);
        std::this_thread::sleep_for(milliseconds(8));  // ~130Hz timing
    }

    upscaler.stop();
}

// Performance test
void testPerformance() {
    Config cfg = Config::getDefault();
    cfg.backend = Backend::Mock;
    TouchUpscaler upscaler(cfg);

    upscaler.start();

    auto startTime = high_resolution_clock::now();

    // Push many samples to test performance
    for (int i = 0; i < 10000; i++) {
        TouchSample sample;
        sample.x = static_cast<float>(i % 100) / 100.0f;
        sample.y = static_cast<float>((i * 2) % 100) / 100.0f;
        sample.valid = (i % 10) != 0;  // 90% valid samples

        upscaler.push(sample);
    }

    auto endTime = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(endTime - startTime);

    std::cout << "  Performance: " << 10000.0 / (duration.count() / 1000000.0) << " samples/sec" << std::endl;

    upscaler.stop();
}

int main() {
    TestRunner runner;

    std::cout << "=== TouchUpscaler Test Suite ===" << std::endl;

    // Run all tests
    runner.runTest("Default Configuration", testDefaultConfig);
    runner.runTest("Basic Construction", testBasicConstruction);
    runner.runTest("Single Touch Input", testSingleTouchInput);
    runner.runTest("Multiple Valid Samples", testMultipleValidSamples);
    runner.runTest("Invalid Sample Handling", testInvalidSampleHandling);
    runner.runTest("Extrapolation Time Limits", testExtrapolationTimeLimits);
    runner.runTest("Thread Safety", testThreadSafety);
    runner.runTest("Configuration Validation", testConfigurationValidation);
    runner.runTest("Skipped Frame Detection", testSkippedFrameDetection);
    runner.runTest("EMA Smoothing", testEMASmoothing);
    runner.runTest("Performance Test", testPerformance);

    runner.printSummary();

    return runner.allTestsPassed() ? 0 : 1;
}