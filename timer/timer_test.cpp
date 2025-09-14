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
        testRecursiveTimerAddition();

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

    void testRecursiveTimerAddition() {
        std::cout << "\n--- Test 5: Recursive Timer Addition (Callback Safety) ---" << std::endl;

        Timer timer;
        std::vector<int> execution_order;
        auto start_time = std::chrono::steady_clock::now();
        int chain_count = 0;
        const int max_chain_count = 5;

        // Helper function to log execution with timing
        auto logExecution = [&](int timer_id) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            std::cout << "Timer " << timer_id << " executed after " << elapsed << "ms" << std::endl;
            execution_order.push_back(timer_id);
        };

        // Test 1: Timer that adds another timer in its callback
        std::cout << "Test 5a: Timer adding new timer in callback" << std::endl;
        timer.addTimer(std::chrono::milliseconds(100), [&]() {
            logExecution(1);
            std::cout << "  -> Timer 1 adding Timer 4 (50ms from now)" << std::endl;

            timer.addTimer(std::chrono::milliseconds(50), [&]() {
                logExecution(4);
            });
        });

        // Test 2: Multiple timers that expire simultaneously and add new timers
        timer.addTimer(std::chrono::milliseconds(200), [&]() {
            logExecution(2);
            std::cout << "  -> Timer 2 adding Timer 5 (30ms from now)" << std::endl;

            timer.addTimer(std::chrono::milliseconds(30), [&]() {
                logExecution(5);
            });
        });

        timer.addTimer(std::chrono::milliseconds(200), [&]() {
            logExecution(3);
            std::cout << "  -> Timer 3 adding Timer 6 (40ms from now)" << std::endl;

            timer.addTimer(std::chrono::milliseconds(40), [&]() {
                logExecution(6);
            });
        });

        // Test 3: Chained timer creation (each timer creates the next one)
        std::cout << "Test 5b: Chained timer creation" << std::endl;
        std::function<void()> createChainedTimer = [&]() {
            chain_count++;
            int current_id = 10 + chain_count;
            logExecution(current_id);

            if (chain_count < max_chain_count) {
                std::cout << "  -> Chained timer " << current_id << " adding timer " << (current_id + 1) << std::endl;
                timer.addTimer(std::chrono::milliseconds(50), createChainedTimer);
            } else {
                std::cout << "  -> Chain completed at timer " << current_id << std::endl;
            }
        };

        timer.addTimer(std::chrono::milliseconds(300), createChainedTimer);

        std::cout << "Initial active timers: " << timer.getActiveTimerCount() << std::endl;

        // Run the timer system and monitor execution
        int iterations = 0;
        const int max_iterations = 100; // Safety limit

        while (timer.hasActiveTimers() && iterations < max_iterations) {
            auto before_count = timer.getActiveTimerCount();
            timer.update();
            auto after_count = timer.getActiveTimerCount();

            // Print status every 10 iterations or when timer count changes significantly
            if (iterations % 10 == 0 || (before_count > 0 && after_count != before_count)) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time).count();
                std::cout << "After " << elapsed << "ms - Active timers: " << after_count
                          << " (was " << before_count << ")" << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            iterations++;
        }

        // Final summary
        std::cout << "\nRecursive timer test completed!" << std::endl;
        std::cout << "Total iterations: " << iterations << std::endl;
        std::cout << "Final active timers: " << timer.getActiveTimerCount() << std::endl;
        std::cout << "Execution order: ";
        for (int id : execution_order) {
            std::cout << id << " ";
        }
        std::cout << std::endl;

        // Verify expected behavior
        bool test_passed = true;

        // Check that original timers executed in correct order (1, then 2&3 together since they have same delay)
        if (execution_order.size() >= 3) {
            if (execution_order[0] != 1) {
                std::cout << "ERROR: Timer 1 should execute first!" << std::endl;
                test_passed = false;
            }
            // Timers 2 and 3 both have 200ms delay, so they should execute together but order may vary
            bool found_2_and_3 = false;
            for (size_t i = 1; i < execution_order.size() && i < 3; ++i) {
                if ((execution_order[i] == 2 || execution_order[i] == 3)) {
                    found_2_and_3 = true;
                    break;
                }
            }
            if (!found_2_and_3) {
                std::cout << "ERROR: Timers 2 and 3 should execute after timer 1!" << std::endl;
                test_passed = false;
            }
        } else {
            std::cout << "ERROR: Not enough timers executed!" << std::endl;
            test_passed = false;
        }

        // Check that recursively added timers executed
        bool found_4 = false, found_5 = false, found_6 = false;
        for (int id : execution_order) {
            if (id == 4) found_4 = true;
            if (id == 5) found_5 = true;
            if (id == 6) found_6 = true;
        }

        if (!found_4 || !found_5 || !found_6) {
            std::cout << "ERROR: Some recursively added timers didn't execute!" << std::endl;
            test_passed = false;
        }

        // Check that chained timers executed
        int chain_timers_found = 0;
        for (int id : execution_order) {
            if (id >= 11 && id <= 15) {
                chain_timers_found++;
            }
        }

        if (chain_timers_found != max_chain_count) {
            std::cout << "ERROR: Expected " << max_chain_count << " chained timers, found " << chain_timers_found << std::endl;
            test_passed = false;
        }

        std::cout << "Recursive timer addition test: " << (test_passed ? "PASSED" : "FAILED") << std::endl;
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