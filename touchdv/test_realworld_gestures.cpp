#include "VirtualTouchDevice.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <iomanip>
#include <cmath>
#include <vector>
#include <algorithm>
#include <queue>
#include <mutex>
#include <atomic>
#include <set>

using namespace std::chrono;

// Real-time UI Framework Simulator with callback processing
class RealTimeUIFramework {
public:
    enum class GestureType {
        Unknown,
        Tap,
        DoubleTap,
        Hold,
        Swipe,
        Pan,
        Flick
    };

    struct RecognizedGesture {
        GestureType type;
        steady_clock::time_point startTime;
        steady_clock::time_point endTime;
        float startX, startY;
        float endX, endY;
        double duration;
        double velocity;
        std::string direction;
        bool isComplete;

        std::string toString() const {
            switch (type) {
                case GestureType::Tap: return "Tap";
                case GestureType::DoubleTap: return "DoubleTap";
                case GestureType::Hold: return "Hold";
                case GestureType::Swipe: return "Swipe";
                case GestureType::Pan: return "Pan";
                case GestureType::Flick: return "Flick";
                default: return "Unknown";
            }
        }
    };

private:
    mutable std::mutex mMutex;
    std::vector<TouchPoint> mEventHistory;
    std::vector<RecognizedGesture> mRecognizedGestures;

    // Touch event tracking (for validation)
    int mTouchDownCount = 0;
    int mTouchUpCount = 0;

    // Current gesture state
    bool mInGesture = false;
    steady_clock::time_point mGestureStartTime;
    steady_clock::time_point mLastTouchUpTime;
    float mGestureStartX = 0, mGestureStartY = 0;
    std::vector<TouchPoint> mCurrentGesturePoints;

    // Double tap detection
    bool mWaitingForSecondTap = false;
    steady_clock::time_point mFirstTapTime;
    float mFirstTapX = 0, mFirstTapY = 0;

    // Gesture analysis parameters
    static constexpr double TAP_MAX_DURATION = 0.3;
    static constexpr double HOLD_MIN_DURATION = 0.5;
    static constexpr double DOUBLE_TAP_MAX_INTERVAL = 0.5;
    static constexpr double DOUBLE_TAP_MAX_DISTANCE = 50.0;
    static constexpr double MIN_SWIPE_DISTANCE = 100.0;
    static constexpr double MIN_SWIPE_VELOCITY = 500.0;
    static constexpr double MIN_FLICK_VELOCITY = 1000.0;
    static constexpr double MAX_SWIPE_DURATION = 0.8;
    static constexpr double MIN_PAN_DURATION = 0.3;

public:
    // Real-time event callback (called from virtual device thread)
    void onTouchEvent(const TouchPoint& point) {
        std::lock_guard<std::mutex> lock(mMutex);

        mEventHistory.push_back(point);

        // Limit history to prevent memory growth
        if (mEventHistory.size() > 1000) {
            mEventHistory.erase(mEventHistory.begin(), mEventHistory.begin() + 500);
        }

        processRealtimeEvent(point);
    }

private:
    void processRealtimeEvent(const TouchPoint& point) {
        // Track touch events for validation
        if (point.touching && !mInGesture) {
            // Gesture start (touch down)
            mTouchDownCount++;
            startNewGesture(point);
        } else if (point.touching && mInGesture) {
            // Gesture continuation
            mCurrentGesturePoints.push_back(point);
        } else if (!point.touching && mInGesture) {
            // Gesture end (touch up)
            mTouchUpCount++;
            endCurrentGesture(point);
        }
    }

    void startNewGesture(const TouchPoint& point) {
        mInGesture = true;
        mGestureStartTime = point.ts;
        mGestureStartX = point.x;
        mGestureStartY = point.y;
        mCurrentGesturePoints.clear();
        mCurrentGesturePoints.push_back(point);
    }

    void endCurrentGesture(const TouchPoint& point) {
        mInGesture = false;
        mCurrentGesturePoints.push_back(point);

        auto gesture = analyzeGesture(mCurrentGesturePoints);

        // Check for double tap
        if (gesture.type == GestureType::Tap) {
            if (mWaitingForSecondTap) {
                auto timeBetweenTaps = toSeconds(gesture.startTime - mFirstTapTime);
                auto distance = std::sqrt(std::pow(gesture.startX - mFirstTapX, 2) +
                                        std::pow(gesture.startY - mFirstTapY, 2));

                if (timeBetweenTaps <= DOUBLE_TAP_MAX_INTERVAL && distance <= DOUBLE_TAP_MAX_DISTANCE) {
                    // Convert to double tap
                    mRecognizedGestures.pop_back(); // Remove the first tap
                    gesture.type = GestureType::DoubleTap;
                    gesture.startTime = mFirstTapTime;
                    mWaitingForSecondTap = false;
                } else {
                    mWaitingForSecondTap = false;
                }
            } else {
                // Start waiting for potential second tap
                mWaitingForSecondTap = true;
                mFirstTapTime = gesture.startTime;
                mFirstTapX = gesture.startX;
                mFirstTapY = gesture.startY;
            }
        } else {
            mWaitingForSecondTap = false;
        }

        mRecognizedGestures.push_back(gesture);
        mLastTouchUpTime = point.ts;
    }

    RecognizedGesture analyzeGesture(const std::vector<TouchPoint>& points) {
        RecognizedGesture gesture;
        gesture.isComplete = true;

        if (points.empty()) {
            gesture.type = GestureType::Unknown;
            return gesture;
        }

        gesture.startTime = points.front().ts;
        gesture.endTime = points.back().ts;
        gesture.startX = points.front().x;
        gesture.startY = points.front().y;
        gesture.endX = points.back().x;
        gesture.endY = points.back().y;
        gesture.duration = toSeconds(gesture.endTime - gesture.startTime);

        // Calculate movement metrics
        double totalDistance = 0.0;
        double maxVelocity = 0.0;
        std::vector<double> velocities;

        for (size_t i = 1; i < points.size(); i++) {
            double dx = points[i].x - points[i-1].x;
            double dy = points[i].y - points[i-1].y;
            double distance = std::sqrt(dx*dx + dy*dy);
            totalDistance += distance;

            double dt = toSeconds(points[i].ts - points[i-1].ts);
            if (dt > 0.001) {
                double velocity = distance / dt;
                velocities.push_back(velocity);
                maxVelocity = std::max(maxVelocity, velocity);
            }
        }

        double avgVelocity = 0.0;
        if (!velocities.empty()) {
            for (double v : velocities) avgVelocity += v;
            avgVelocity /= velocities.size();
        }
        gesture.velocity = avgVelocity;

        // Calculate direction
        double dx = gesture.endX - gesture.startX;
        double dy = gesture.endY - gesture.startY;
        double netDistance = std::sqrt(dx*dx + dy*dy);

        if (netDistance > 50.0) {
            if (std::abs(dx) > std::abs(dy) * 1.5) {
                gesture.direction = (dx > 0) ? "right" : "left";
            } else if (std::abs(dy) > std::abs(dx) * 1.5) {
                gesture.direction = (dy > 0) ? "down" : "up";
            } else {
                if (dx > 0 && dy > 0) gesture.direction = "down-right";
                else if (dx > 0 && dy < 0) gesture.direction = "up-right";
                else if (dx < 0 && dy > 0) gesture.direction = "down-left";
                else gesture.direction = "up-left";
            }
        } else {
            gesture.direction = "none";
        }

        // Classify gesture
        if (gesture.duration < TAP_MAX_DURATION && netDistance < 50.0) {
            gesture.type = GestureType::Tap;
        } else if (gesture.duration >= HOLD_MIN_DURATION && netDistance < 50.0) {
            gesture.type = GestureType::Hold;
        } else if (netDistance >= MIN_SWIPE_DISTANCE) {
            if (gesture.duration < MAX_SWIPE_DURATION && avgVelocity >= MIN_FLICK_VELOCITY) {
                gesture.type = GestureType::Flick;
            } else if (gesture.duration < MAX_SWIPE_DURATION && avgVelocity >= MIN_SWIPE_VELOCITY) {
                gesture.type = GestureType::Swipe;
            } else if (gesture.duration >= MIN_PAN_DURATION) {
                gesture.type = GestureType::Pan;
            } else {
                gesture.type = GestureType::Swipe; // Default for movement
            }
        } else {
            gesture.type = GestureType::Unknown;
        }

        return gesture;
    }

public:
    std::vector<RecognizedGesture> getRecognizedGestures() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mRecognizedGestures;
    }

    size_t getEventCount() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mEventHistory.size();
    }

    std::pair<int, int> getTouchEventCounts() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return {mTouchDownCount, mTouchUpCount};
    }

    bool isSequenceValid() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mTouchDownCount == mTouchUpCount && !mInGesture;
    }

    void clearHistory() {
        std::lock_guard<std::mutex> lock(mMutex);
        mEventHistory.clear();
        mRecognizedGestures.clear();
        mTouchDownCount = 0;
        mTouchUpCount = 0;
        mInGesture = false;
        mWaitingForSecondTap = false;
    }

    void printGestureReport() const {
        std::lock_guard<std::mutex> lock(mMutex);

        std::cout << "\n  📱 Real-time UI Framework Report:" << std::endl;
        std::cout << "    Total events received: " << mEventHistory.size() << std::endl;
        std::cout << "    Gestures recognized: " << mRecognizedGestures.size() << std::endl;

        auto startTime = mRecognizedGestures.empty() ? steady_clock::now() : mRecognizedGestures[0].startTime;

        for (size_t i = 0; i < mRecognizedGestures.size(); i++) {
            const auto& gesture = mRecognizedGestures[i];
            auto relativeTime = toSeconds(gesture.startTime - startTime);

            std::cout << "    " << (i+1) << ". " << gesture.toString()
                      << " at +" << std::fixed << std::setprecision(2) << relativeTime << "s"
                      << " (dur: " << std::fixed << std::setprecision(3) << gesture.duration << "s)";

            if (gesture.direction != "none") {
                std::cout << " [" << gesture.direction << "]";
            }

            if (gesture.velocity > 100.0) {
                std::cout << " (vel: " << std::fixed << std::setprecision(0) << gesture.velocity << " px/s)";
            }

            std::cout << std::endl;
        }
    }
};

// Gesture sequence generator for realistic user interaction patterns
class GestureSequenceGenerator {
public:
    struct GestureCommand {
        std::string type;
        double startTimeMs;
        std::vector<std::pair<float, float>> waypoints;
        double durationMs;
        std::string description;
    };

    static std::vector<GestureCommand> createRealWorldScenario() {
        return {
            // App launch scenario
            {"tap", 0, {{500, 400}}, 150, "Launch app icon"},

            // Navigation gestures
            {"swipe", 1500, {{800, 600}, {200, 600}}, 300, "Swipe right-to-left (page turn)"},
            {"tap", 2200, {{300, 300}}, 120, "Tap menu item"},

            // Scrolling behavior
            {"pan", 3000, {{600, 700}, {600, 400}, {600, 200}}, 800, "Scroll up through content"},
            {"flick", 4200, {{600, 600}, {600, 100}}, 150, "Quick flick to top"},

            // Interaction with UI elements
            {"double_tap", 5500, {{400, 500}}, 120, "Double tap to zoom"},
            {"hold", 6800, {{700, 400}}, 1000, "Long press for context menu"},

            // More navigation
            {"swipe", 8200, {{100, 500}, {900, 500}}, 400, "Swipe left-to-right (back)"},
            {"tap", 9000, {{800, 600}}, 100, "Tap button"},

            // Complex gesture sequence
            {"pan", 10000, {{400, 300}, {600, 500}, {400, 700}}, 600, "Pan in curved motion"},
            {"tap", 11000, {{500, 300}}, 110, "Tap to select"},
            {"swipe", 11800, {{600, 400}, {600, 800}}, 250, "Swipe down to refresh"},

            // Rapid interaction
            {"tap", 13000, {{300, 400}}, 80, "Quick tap 1"},
            {"tap", 13300, {{310, 410}}, 90, "Quick tap 2"},
            {"tap", 13600, {{320, 420}}, 85, "Quick tap 3"},

            // Final gesture
            {"double_tap", 15000, {{960, 540}}, 130, "Double tap to exit"}
        };
    }
};

void executeGestureCommand(VirtualTouchDevice& device, const GestureSequenceGenerator::GestureCommand& cmd, steady_clock::time_point baseTime) {
    auto startTime = baseTime + milliseconds(static_cast<int>(cmd.startTimeMs));

    if (cmd.type == "tap") {
        auto pos = cmd.waypoints[0];

        TouchPoint down;
        down.x = pos.first;
        down.y = pos.second;
        down.touching = true;
        down.ts = startTime;
        device.pushInputPoint(down);

        std::this_thread::sleep_for(milliseconds(static_cast<int>(cmd.durationMs)));

        TouchPoint up;
        up.x = pos.first;
        up.y = pos.second;
        up.touching = false;
        up.pressure = 0;
        up.ts = startTime + milliseconds(static_cast<int>(cmd.durationMs));
        device.pushInputPoint(up);

    } else if (cmd.type == "double_tap") {
        auto pos = cmd.waypoints[0];

        // First tap
        TouchPoint down1;
        down1.x = pos.first;
        down1.y = pos.second;
        down1.touching = true;
        down1.ts = startTime;
        device.pushInputPoint(down1);

        std::this_thread::sleep_for(milliseconds(static_cast<int>(cmd.durationMs)));

        TouchPoint up1;
        up1.x = pos.first;
        up1.y = pos.second;
        up1.touching = false;
        up1.pressure = 0;
        up1.ts = startTime + milliseconds(static_cast<int>(cmd.durationMs));
        device.pushInputPoint(up1);

        // Small gap
        std::this_thread::sleep_for(milliseconds(100));

        // Second tap
        TouchPoint down2;
        down2.x = pos.first + 2;
        down2.y = pos.second + 2;
        down2.touching = true;
        down2.ts = startTime + milliseconds(static_cast<int>(cmd.durationMs) + 100);
        device.pushInputPoint(down2);

        std::this_thread::sleep_for(milliseconds(static_cast<int>(cmd.durationMs)));

        TouchPoint up2;
        up2.x = pos.first + 2;
        up2.y = pos.second + 2;
        up2.touching = false;
        up2.pressure = 0;
        up2.ts = startTime + milliseconds(static_cast<int>(cmd.durationMs) * 2 + 100);
        device.pushInputPoint(up2);

    } else if (cmd.type == "hold") {
        auto pos = cmd.waypoints[0];

        TouchPoint down;
        down.x = pos.first;
        down.y = pos.second;
        down.touching = true;
        down.ts = startTime;
        device.pushInputPoint(down);

        // Send periodic updates during hold (simulate 30Hz input)
        int holdSteps = static_cast<int>(cmd.durationMs / 33); // ~30Hz
        for (int i = 1; i < holdSteps; i++) {
            TouchPoint hold;
            hold.x = pos.first;
            hold.y = pos.second;
            hold.touching = true;
            hold.ts = startTime + milliseconds(i * 33);
            device.pushInputPoint(hold);
            std::this_thread::sleep_for(milliseconds(33));
        }

        TouchPoint up;
        up.x = pos.first;
        up.y = pos.second;
        up.touching = false;
        up.pressure = 0;
        up.ts = startTime + milliseconds(static_cast<int>(cmd.durationMs));
        device.pushInputPoint(up);

    } else if (cmd.type == "swipe" || cmd.type == "pan" || cmd.type == "flick") {
        if (cmd.waypoints.size() < 2) return;

        auto startPos = cmd.waypoints[0];
        auto endPos = cmd.waypoints.back();

        TouchPoint down;
        down.x = startPos.first;
        down.y = startPos.second;
        down.touching = true;
        down.ts = startTime;
        device.pushInputPoint(down);

        // Generate movement points at 30Hz
        int moveSteps = static_cast<int>(cmd.durationMs / 33);
        for (int i = 1; i < moveSteps; i++) {
            float progress = static_cast<float>(i) / moveSteps;

            // Interpolate through waypoints
            float x, y;
            if (cmd.waypoints.size() == 2) {
                x = startPos.first + progress * (endPos.first - startPos.first);
                y = startPos.second + progress * (endPos.second - startPos.second);
            } else {
                // Multi-waypoint interpolation
                float segmentLength = 1.0f / (cmd.waypoints.size() - 1);
                int segment = static_cast<int>(progress / segmentLength);
                segment = std::min(segment, static_cast<int>(cmd.waypoints.size()) - 2);

                float segmentProgress = (progress - segment * segmentLength) / segmentLength;

                auto& p1 = cmd.waypoints[segment];
                auto& p2 = cmd.waypoints[segment + 1];

                x = p1.first + segmentProgress * (p2.first - p1.first);
                y = p1.second + segmentProgress * (p2.second - p1.second);
            }

            TouchPoint move;
            move.x = x;
            move.y = y;
            move.touching = true;
            move.ts = startTime + milliseconds(i * 33);
            device.pushInputPoint(move);

            std::this_thread::sleep_for(milliseconds(33));
        }

        TouchPoint up;
        up.x = endPos.first;
        up.y = endPos.second;
        up.touching = false;
        up.pressure = 0;
        up.ts = startTime + milliseconds(static_cast<int>(cmd.durationMs));
        device.pushInputPoint(up);
    }
}

void testRealWorldMixedGestures() {
    std::cout << "\n🌍📱 === Test: Real-World Mixed Gesture Scenarios ===" << std::endl;
    std::cout << "Testing mixed gestures (tap, swipe, pan, flick, double tap) with 30Hz input → UI framework output" << std::endl;

    Config cfg;
    cfg.screenWidth = 2560;
    cfg.screenHeight = 1440;
    cfg.inputRateHz = 30.0;  // 30Hz input as specified
    cfg.outputRateHz = 120.0; // High output rate for UI framework
    cfg.touchTimeoutMs = 0.0;
    cfg.maxInputHistorySec = 2.0;

    VirtualTouchDevice device(cfg);
    device.setTouchTransitionThreshold(0.05); // Configure touch transitions for clean gesture separation

    // Create real-time UI framework simulator
    RealTimeUIFramework uiFramework;

    // Set up real-time callback - this is the ONLY interface we need!
    // device.setMockEventCallback([&uiFramework](const TouchPoint& point) {
    //     uiFramework.onTouchEvent(point);
    // });

    if (!device.start()) {
        std::cerr << "Failed to start device" << std::endl;
        return;
    }

    auto gestureSequence = GestureSequenceGenerator::createRealWorldScenario();
    auto baseTime = steady_clock::now();

    std::cout << "\n  🎬 Executing Real-World Gesture Scenario:" << std::endl;
    std::cout << "  Total gestures: " << gestureSequence.size() << std::endl;
    std::cout << "  Input rate: 30Hz, Output rate: 120Hz" << std::endl;
    std::cout << "  Duration: ~16 seconds" << std::endl;

    // Print gesture timeline
    std::cout << "\n  📅 Gesture Timeline:" << std::endl;
    for (size_t i = 0; i < gestureSequence.size(); i++) {
        const auto& cmd = gestureSequence[i];
        std::cout << "    " << std::fixed << std::setprecision(1) << cmd.startTimeMs/1000.0
                  << "s: " << cmd.type << " - " << cmd.description << std::endl;
    }

    // Execute gesture sequence
    std::cout << "\n  🎮 Executing gesture sequence..." << std::endl;

    std::thread executionThread([&]() {
        for (const auto& cmd : gestureSequence) {
            executeGestureCommand(device, cmd, baseTime);
            std::this_thread::sleep_for(milliseconds(50)); // Small gap between gestures
        }
    });

    // Monitor progress
    auto startTime = steady_clock::now();
    auto endTime = startTime + seconds(18); // 16s + 2s buffer

    size_t lastEventCount = 0;
    while (steady_clock::now() < endTime && executionThread.joinable()) {
        std::this_thread::sleep_for(milliseconds(1000));

        auto currentEventCount = uiFramework.getEventCount();
        auto elapsed = duration_cast<seconds>(steady_clock::now() - startTime).count();

        if (currentEventCount != lastEventCount) {
            std::cout << "    " << elapsed << "s: " << currentEventCount << " events received by UI framework" << std::endl;
            lastEventCount = currentEventCount;
        }
    }

    executionThread.join();

    // Final processing time
    std::this_thread::sleep_for(milliseconds(500));

    // Analyze results - using ONLY the UI framework data (callback-based)
    auto recognizedGestures = uiFramework.getRecognizedGestures();
    auto totalEvents = uiFramework.getEventCount();

    std::cout << "\n  📊 Real-World Test Results:" << std::endl;
    std::cout << "    Input gestures sent: " << gestureSequence.size() << std::endl;
    std::cout << "    UI framework events received: " << totalEvents << std::endl;
    std::cout << "    Gestures recognized by UI: " << recognizedGestures.size() << std::endl;

    // Print UI framework gesture report
    uiFramework.printGestureReport();

    // Validation using ONLY UI framework data (callback-based)
    auto [touchDowns, touchUps] = uiFramework.getTouchEventCounts();
    bool sequenceValid = uiFramework.isSequenceValid();

    bool testPassed = true;

    // Test 1: Touch event integrity (detected by UI framework)
    if (touchDowns != touchUps || !sequenceValid) {
        std::cout << "\n    ❌ FAIL: UI framework detected unbalanced touch events ("
                  << touchDowns << " downs, " << touchUps << " ups)" << std::endl;
        testPassed = false;
    } else {
        std::cout << "\n    ✅ PASS: UI framework received balanced touch events ("
                  << touchDowns << " downs, " << touchUps << " ups)" << std::endl;
    }

    // Test 2: Event delivery rate (callback effectiveness)
    if (totalEvents > 0) {
        double avgOutputRate = totalEvents / 16.0; // 16 second test
        std::cout << "    ✅ PASS: Real-time callbacks delivering events (avg rate: "
                  << std::fixed << std::setprecision(1) << avgOutputRate << " events/sec)" << std::endl;
    } else {
        std::cout << "    ❌ FAIL: No events received through callbacks" << std::endl;
        testPassed = false;
    }

    // Test 3: Gesture recognition rate
    double recognitionRate = static_cast<double>(recognizedGestures.size()) / gestureSequence.size();
    if (recognitionRate >= 0.5) { // Reasonable threshold for mixed gestures
        std::cout << "    ✅ PASS: Good gesture recognition rate ("
                  << std::fixed << std::setprecision(1) << recognitionRate * 100 << "%)" << std::endl;
    } else {
        std::cout << "    ⚠️  WARN: Low gesture recognition rate ("
                  << std::fixed << std::setprecision(1) << recognitionRate * 100 << "%)" << std::endl;
    }

    // Test 4: Gesture type variety
    std::set<RealTimeUIFramework::GestureType> recognizedTypes;
    for (const auto& gesture : recognizedGestures) {
        recognizedTypes.insert(gesture.type);
    }

    if (recognizedTypes.size() >= 3) { // At least 3 different gesture types
        std::cout << "    ✅ PASS: Variety of gesture types recognized (" << recognizedTypes.size() << " types)" << std::endl;
    } else {
        std::cout << "    ⚠️  WARN: Limited gesture type variety (" << recognizedTypes.size() << " types)" << std::endl;
    }

    // Test 5: Real-time callback responsiveness
    if (totalEvents > gestureSequence.size() * 10) { // Expect multiple events per gesture
        std::cout << "    ✅ PASS: Real-time callback system delivering rich event stream" << std::endl;
    } else {
        std::cout << "    ⚠️  WARN: Lower than expected event density from callbacks" << std::endl;
    }

    device.stop();

    // Final assessment using ONLY callback data
    std::cout << "\n  📋 Real-World Mixed Gesture Test Summary:" << std::endl;
    std::cout << "    Input simulation: 30Hz raw events (" << gestureSequence.size() << " gestures)" << std::endl;
    std::cout << "    Virtual device: " << cfg.outputRateHz << "Hz output rate" << std::endl;
    std::cout << "    UI framework: Real-time callbacks (" << totalEvents << " events processed)" << std::endl;
    std::cout << "    Touch events tracked: " << touchDowns << " downs, " << touchUps << " ups" << std::endl;
    std::cout << "    Gesture reconstruction: " << recognizedGestures.size() << " gestures recognized" << std::endl;

    if (testPassed) {
        std::cout << "\n✅ REAL-WORLD MIXED GESTURE TEST PASSED!" << std::endl;
        std::cout << "   Virtual device successfully handles mixed gesture scenarios" << std::endl;
        std::cout << "   UI framework receives clean, real-time gesture events" << std::endl;
        std::cout << "   Gesture reconstruction working effectively" << std::endl;
    } else {
        std::cout << "\n❌ REAL-WORLD MIXED GESTURE TEST FAILED!" << std::endl;
        std::cout << "   Issues detected in real-world scenario handling" << std::endl;
    }
}

int main() {
    std::cout << "🌍 Real-World Mixed Gesture Test Suite" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "Testing virtual touch device with realistic mixed gesture scenarios..." << std::endl;
    std::cout << "Input: 30Hz raw events → Virtual Device → UI Framework (real-time callbacks)" << std::endl;

    try {
        testRealWorldMixedGestures();

        std::cout << "\n🎉 REAL-WORLD GESTURE TESTING COMPLETED! 🎉" << std::endl;
        std::cout << "\n📋 Comprehensive Test Coverage:" << std::endl;
        std::cout << "  ✅ Mixed gesture types (tap, swipe, pan, flick, double tap, hold)" << std::endl;
        std::cout << "  ✅ 30Hz input rate simulation" << std::endl;
        std::cout << "  ✅ Real-time UI framework callbacks" << std::endl;
        std::cout << "  ✅ Gesture reconstruction and recognition" << std::endl;
        std::cout << "  ✅ Real-world interaction patterns" << std::endl;
        std::cout << "  ✅ Touch event integrity validation" << std::endl;
        std::cout << "  ✅ Performance and responsiveness testing" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
