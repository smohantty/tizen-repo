#include "timer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

class TimerTester {
public:
    void runTests() {
        std::cout << "=== Real-Time Timer Test Suite ===" << std::endl;

        testBasicTimers();
        testMultipleTimers();
        testTimerOrdering();
        testSecondsAndMilliseconds();

        std::cout << "\n=== All Tests Completed ===" << std::endl;
    }

private:
    void testBasicTimers() {
        std::cout << "\n--- Test 1: Basic Timer Functionality ---" << std::endl;

        Timer timer;
        bool timer1_fired = false;
        bool timer2_fired = false;
        auto start_time = std::chrono::steady_clock::now();

        // Add timers
        auto id1 = timer.addTimer(std::chrono::milliseconds(100), [&]() {
            timer1_fired = true;
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            std::cout << "Timer 1 (100ms) fired after " << elapsed << "ms!" << std::endl;
        });

        auto id2 = timer.addTimer(std::chrono::milliseconds(200), [&]() {
            timer2_fired = true;
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            std::cout << "Timer 2 (200ms) fired after " << elapsed << "ms!" << std::endl;
        });

        std::cout << "Added timers with IDs: " << id1 << ", " << id2 << std::endl;
        std::cout << "Active timers: " << timer.getActiveTimerCount() << std::endl;

        // Check timers periodically for 250ms
        for (int i = 0; i < 25; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            timer.update();

            if (i % 5 == 0) { // Print every 50ms
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time).count();
                std::cout << "After " << elapsed << "ms - Timer 1: " << (timer1_fired ? "Yes" : "No")
                          << ", Timer 2: " << (timer2_fired ? "Yes" : "No")
                          << ", Active: " << timer.getActiveTimerCount() << std::endl;
            }

            if (timer1_fired && timer2_fired) break;
        }

        std::cout << "Final active timers: " << timer.getActiveTimerCount() << std::endl;
    }

    void testMultipleTimers() {
        std::cout << "\n--- Test 2: Multiple Timers with Different Intervals ---" << std::endl;

        Timer timer;
        std::vector<bool> timer_fired(5, false);
        auto start_time = std::chrono::steady_clock::now();

        // Add multiple timers with different intervals
        for (int i = 0; i < 5; ++i) {
            auto delay = std::chrono::milliseconds((i + 1) * 50); // 50ms, 100ms, 150ms, 200ms, 250ms
            timer.addTimer(delay, [&, i]() {
                timer_fired[i] = true;
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time).count();
                std::cout << "Timer " << (i + 1) << " (" << (i + 1) * 50 << "ms) fired after "
                          << elapsed << "ms!" << std::endl;
            });
        }

        std::cout << "Added 5 timers with intervals: 50ms, 100ms, 150ms, 200ms, 250ms" << std::endl;
        std::cout << "Active timers: " << timer.getActiveTimerCount() << std::endl;

        // Check timers until all fire or timeout
        for (int i = 0; i < 30; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            timer.update();

            if (i % 5 == 0) { // Print every 50ms
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time).count();
                std::cout << "After " << elapsed << "ms - Fired timers: ";
                for (size_t j = 0; j < timer_fired.size(); ++j) {
                    if (timer_fired[j]) std::cout << (j + 1) << " ";
                }
                std::cout << "| Active: " << timer.getActiveTimerCount() << std::endl;
            }

            // Check if all timers fired
            bool all_fired = true;
            for (bool fired : timer_fired) {
                if (!fired) {
                    all_fired = false;
                    break;
                }
            }
            if (all_fired) break;
        }
    }

    void testTimerOrdering() {
        std::cout << "\n--- Test 3: Timer Execution Order ---" << std::endl;

        Timer timer;
        std::vector<int> execution_order;
        auto start_time = std::chrono::steady_clock::now();

        // Add timers in reverse chronological order to test min-heap
        timer.addTimer(std::chrono::milliseconds(150), [&]() {
            execution_order.push_back(3);
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            std::cout << "Timer 3 (150ms) executed after " << elapsed << "ms" << std::endl;
        });

        timer.addTimer(std::chrono::milliseconds(50), [&]() {
            execution_order.push_back(1);
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            std::cout << "Timer 1 (50ms) executed after " << elapsed << "ms" << std::endl;
        });

        timer.addTimer(std::chrono::milliseconds(100), [&]() {
            execution_order.push_back(2);
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            std::cout << "Timer 2 (100ms) executed after " << elapsed << "ms" << std::endl;
        });

        std::cout << "Added timers in order: 150ms, 50ms, 100ms" << std::endl;

        // Check timers until all execute
        while (timer.hasActiveTimers() && execution_order.size() < 3) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            timer.update();
        }

        std::cout << "Execution order: ";
        for (int order : execution_order) {
            std::cout << order << " ";
        }
        std::cout << std::endl;

        // Verify correct order
        bool correct_order = (execution_order.size() == 3 &&
                            execution_order[0] == 1 &&
                            execution_order[1] == 2 &&
                            execution_order[2] == 3);
        std::cout << "Order is correct: " << (correct_order ? "Yes" : "No") << std::endl;
    }

    void testSecondsAndMilliseconds() {
        std::cout << "\n--- Test 4: Seconds and Milliseconds Support ---" << std::endl;

        Timer timer;
        bool seconds_timer_fired = false;
        bool ms_timer_fired = false;
        auto start_time = std::chrono::steady_clock::now();

        // Add timer with seconds
        timer.addTimer(std::chrono::seconds(1), [&]() {
            seconds_timer_fired = true;
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            std::cout << "1-second timer fired after " << elapsed << "ms!" << std::endl;
        });

        // Add timer with milliseconds
        timer.addTimer(std::chrono::milliseconds(500), [&]() {
            ms_timer_fired = true;
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            std::cout << "500ms timer fired after " << elapsed << "ms!" << std::endl;
        });

        std::cout << "Added 1s timer and 500ms timer" << std::endl;

        // Check timers for up to 1.2 seconds
        for (int i = 0; i < 120; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            timer.update();

            if (i % 25 == 0) { // Print every 250ms
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time).count();
                std::cout << "After " << elapsed << "ms - 500ms fired: " << (ms_timer_fired ? "Yes" : "No")
                          << ", 1s fired: " << (seconds_timer_fired ? "Yes" : "No") << std::endl;
            }

            if (seconds_timer_fired && ms_timer_fired) break;
        }
    }
};

void demonstrateRealTimeUsage() {
    std::cout << "\n=== Real-time Usage Demonstration ===" << std::endl;

    Timer timer;
    auto start_time = std::chrono::steady_clock::now();

    // Add some timers
    timer.addTimer(std::chrono::milliseconds(300), []() {
        std::cout << "Short timer (300ms) executed!" << std::endl;
    });

    timer.addTimer(std::chrono::milliseconds(600), []() {
        std::cout << "Medium timer (600ms) executed!" << std::endl;
    });

    timer.addTimer(std::chrono::milliseconds(900), []() {
        std::cout << "Long timer (900ms) executed!" << std::endl;
    });

    std::cout << "Running real-time simulation for 1 second..." << std::endl;

    // Simulate real-time updates
    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(1)) {
        timer.update();

        // Small sleep to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Final update to catch any remaining timers
    timer.update();

    std::cout << "Real-time demonstration completed!" << std::endl;
}

int main() {
    TimerTester tester;
    tester.runTests();

    demonstrateRealTimeUsage();

    return 0;
}