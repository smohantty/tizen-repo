#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <random>
#include "VirtualTouchDevice.h"

using namespace std::chrono;

// Gesture types
enum class GestureType {
    Tap,
    DoubleTap,
    Flick,
    Swipe,
    Pan
};

// Gesture parameters
// Note: All gesture movements use 30fps timing (33ms intervals) for consistent upsampling testing
struct GestureParams {
    float x = 640.0f;           // Start X coordinate
    float y = 360.0f;           // Start Y coordinate
    float endX = 0.0f;          // End X coordinate (for swipe/pan)
    float endY = 0.0f;          // End Y coordinate (for swipe/pan)
    int durationMs = 100;       // Gesture duration in milliseconds
    int speedPxPerSec = 1000;   // Speed for flick/swipe gestures
    int distancePx = 200;       // Distance for flick/swipe gestures
    std::string direction = "right"; // Direction for flick/swipe (up, down, left, right)
};

class RandomParameterGenerator {
private:
    std::random_device mRd;
    std::mt19937 mGen;
    const Config& mCfg;

public:
    explicit RandomParameterGenerator(const Config& cfg) : mGen(mRd()), mCfg(cfg) {}

    GestureParams generateRandomParams(GestureType type) {
        GestureParams params;

        // Random start position within screen bounds (avoiding edges)
        std::uniform_int_distribution<int> xDist(50, mCfg.screenWidth - 50);
        std::uniform_int_distribution<int> yDist(50, mCfg.screenHeight - 50);
        params.x = static_cast<float>(xDist(mGen));
        params.y = static_cast<float>(yDist(mGen));

        switch (type) {
            case GestureType::Tap:
            case GestureType::DoubleTap: {
                // Random duration between 50-200ms
                std::uniform_int_distribution<int> durationDist(50, 200);
                params.durationMs = durationDist(mGen);
                break;
            }

            case GestureType::Flick:
            case GestureType::Swipe: {
                // Random direction
                std::vector<std::string> directions = {"up", "down", "left", "right"};
                std::uniform_int_distribution<int> dirDist(0, directions.size() - 1);
                params.direction = directions[dirDist(mGen)];

                // Random distance between 100-400px
                std::uniform_int_distribution<int> distDist(100, 400);
                params.distancePx = distDist(mGen);

                if (type == GestureType::Swipe) {
                    // Random duration between 200-800ms for swipe
                    std::uniform_int_distribution<int> durationDist(200, 800);
                    params.durationMs = durationDist(mGen);
                } else {
                    // Flick is always fast
                    params.durationMs = 100;
                }
                break;
            }

            case GestureType::Pan: {
                // Random end position within screen bounds
                std::uniform_int_distribution<int> endXDist(50, mCfg.screenWidth - 50);
                std::uniform_int_distribution<int> endYDist(50, mCfg.screenHeight - 50);
                params.endX = static_cast<float>(endXDist(mGen));
                params.endY = static_cast<float>(endYDist(mGen));

                // Random duration between 500-1500ms
                std::uniform_int_distribution<int> durationDist(500, 1500);
                params.durationMs = durationDist(mGen);
                break;
            }
        }

        return params;
    }
};

class GestureGenerator {
private:
    VirtualTouchDevice& mDevice;
    Config mCfg;

public:
    explicit GestureGenerator(VirtualTouchDevice& device, const Config& cfg)
        : mDevice(device), mCfg(cfg) {}

    void performTap(const GestureParams& params) {
        std::cout << "Performing tap at (" << params.x << ", " << params.y << ")\n";

        // Touch down
        TouchPoint touch;
        touch.ts = steady_clock::now();
        touch.x = params.x;
        touch.y = params.y;
        touch.touching = true;
        mDevice.pushInputPoint(touch);

        // Hold briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(params.durationMs));

        // Touch up
        TouchPoint release = touch;
        release.ts = steady_clock::now();
        release.touching = false;
        mDevice.pushInputPoint(release);
    }

    void performDoubleTap(const GestureParams& params) {
        std::cout << "Performing double tap at (" << params.x << ", " << params.y << ")\n";

        // First tap
        performTap(params);

        // Short delay between taps
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Second tap
        performTap(params);
    }

    void performFlick(const GestureParams& params) {
        float endX, endY;
        calculateEndPosition(params.x, params.y, params.direction, params.distancePx, endX, endY);

        std::cout << "Performing flick from (" << params.x << ", " << params.y
                  << ") to (" << endX << ", " << endY << ") - " << params.direction << "\n";

        // Quick movement with immediate release
        TouchPoint start;
        start.ts = steady_clock::now();
        start.x = params.x;
        start.y = params.y;
        start.touching = true;
        mDevice.pushInputPoint(start);

        // Brief movement duration for flick
        int steps = 3; // Reduced steps but maintain 30fps timing
        int stepDelay = 33; // 30fps for upsampling testing

        for (int i = 1; i <= steps; ++i) {
            float progress = float(i) / steps;
            TouchPoint point;
            point.ts = steady_clock::now();
            point.x = params.x + (endX - params.x) * progress;
            point.y = params.y + (endY - params.y) * progress;
            point.touching = true;
            mDevice.pushInputPoint(point);
            std::this_thread::sleep_for(std::chrono::milliseconds(stepDelay));
        }

        // Release
        TouchPoint release;
        release.ts = steady_clock::now();
        release.x = endX;
        release.y = endY;
        release.touching = false;
        mDevice.pushInputPoint(release);
    }

    void performSwipe(const GestureParams& params) {
        float endX, endY;
        calculateEndPosition(params.x, params.y, params.direction, params.distancePx, endX, endY);

        std::cout << "Performing swipe from (" << params.x << ", " << params.y
                  << ") to (" << endX << ", " << endY << ") - " << params.direction << "\n";

        // Touch down
        TouchPoint start;
        start.ts = steady_clock::now();
        start.x = params.x;
        start.y = params.y;
        start.touching = true;
        mDevice.pushInputPoint(start);

        // Smooth movement
        int totalMs = params.durationMs;
        int stepMs = 33; // 30fps for upsampling testing
        int steps = totalMs / stepMs;

        for (int i = 1; i <= steps; ++i) {
            float progress = float(i) / steps;
            TouchPoint point;
            point.ts = steady_clock::now();
            point.x = params.x + (endX - params.x) * progress;
            point.y = params.y + (endY - params.y) * progress;
            point.touching = true;
            mDevice.pushInputPoint(point);
            std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
        }

        // Release
        TouchPoint release;
        release.ts = steady_clock::now();
        release.x = endX;
        release.y = endY;
        release.touching = false;
        mDevice.pushInputPoint(release);
    }

    void performPan(const GestureParams& params) {
        float endX = params.endX != 0.0f ? params.endX : params.x + 100;
        float endY = params.endY != 0.0f ? params.endY : params.y + 100;

        std::cout << "Performing pan from (" << params.x << ", " << params.y
                  << ") to (" << endX << ", " << endY << ")\n";

        // Touch down
        TouchPoint start;
        start.ts = steady_clock::now();
        start.x = params.x;
        start.y = params.y;
        start.touching = true;
        mDevice.pushInputPoint(start);

        // Slow, controlled movement
        int totalMs = params.durationMs;
        int stepMs = 33; // 30fps for upsampling testing
        int steps = totalMs / stepMs;

        for (int i = 1; i <= steps; ++i) {
            float progress = float(i) / steps;
            TouchPoint point;
            point.ts = steady_clock::now();
            point.x = params.x + (endX - params.x) * progress;
            point.y = params.y + (endY - params.y) * progress;
            point.touching = true;
            mDevice.pushInputPoint(point);
            std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
        }

        // Hold at end position briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Release
        TouchPoint release;
        release.ts = steady_clock::now();
        release.x = endX;
        release.y = endY;
        release.touching = false;
        mDevice.pushInputPoint(release);
    }

private:
    void calculateEndPosition(float startX, float startY, const std::string& direction,
                            int distance, float& endX, float& endY) {
        endX = startX;
        endY = startY;

        if (direction == "up") {
            endY = std::max(0.0f, startY - distance);
        } else if (direction == "down") {
            endY = std::min(float(mCfg.screenHeight - 1), startY + distance);
        } else if (direction == "left") {
            endX = std::max(0.0f, startX - distance);
        } else if (direction == "right") {
            endX = std::min(float(mCfg.screenWidth - 1), startX + distance);
        }
    }
};

class CommandParser {
public:
    static std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delimiter)) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    static bool parseGestureCommand(const std::string& input, GestureType& type, GestureParams& params) {
        auto tokens = split(input, ' ');
        if (tokens.empty()) return false;

        std::string command = tokens[0];
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        // Parse gesture type
        if (command == "tap" || command == "t") {
            type = GestureType::Tap;
        } else if (command == "doubletap" || command == "dt") {
            type = GestureType::DoubleTap;
        } else if (command == "flick" || command == "f") {
            type = GestureType::Flick;
        } else if (command == "swipe" || command == "s") {
            type = GestureType::Swipe;
        } else if (command == "pan" || command == "p") {
            type = GestureType::Pan;
        } else {
            return false;
        }

        // Parse parameters
        for (size_t i = 1; i < tokens.size(); ++i) {
            auto& token = tokens[i];
            if (token.find('=') != std::string::npos) {
                auto keyValue = split(token, '=');
                if (keyValue.size() == 2) {
                    std::string key = keyValue[0];
                    std::string value = keyValue[1];

                    if (key == "x") params.x = std::stof(value);
                    else if (key == "y") params.y = std::stof(value);
                    else if (key == "endx") params.endX = std::stof(value);
                    else if (key == "endy") params.endY = std::stof(value);
                    else if (key == "duration") params.durationMs = std::stoi(value);
                    else if (key == "distance") params.distancePx = std::stoi(value);
                    else if (key == "dir" || key == "direction") params.direction = value;
                }
            }
        }

        return true;
    }
};

void printGestureMenu() {
    std::cout << "\n=== TouchDV Random Gesture Generator ===\n";
    std::cout << "Select gesture to emulate:\n";
    std::cout << "  1 - Tap (random position and duration)\n";
    std::cout << "  2 - Double Tap (random position and duration)\n";
    std::cout << "  3 - Flick (random position, direction, and distance)\n";
    std::cout << "  4 - Swipe (random position, direction, distance, and duration)\n";
    std::cout << "  5 - Pan (random start/end positions and duration)\n";
    std::cout << "\nOther commands: help, manual, quit, exit\n\n";
}

void printHelp() {
    std::cout << "\n=== TouchDV Gesture Tester ===\n";
    std::cout << "Two modes available:\n";
    std::cout << "\n1. RANDOM MODE (Default):\n";
    std::cout << "   Enter 1-5 to generate random gestures with random parameters\n";
    std::cout << "\n2. MANUAL MODE:\n";
    std::cout << "   Type 'manual' to switch to manual parameter mode\n";
    std::cout << "   Commands:\n";
    std::cout << "     tap [x=X] [y=Y] [duration=MS]           - Single tap\n";
    std::cout << "     doubletap [x=X] [y=Y] [duration=MS]     - Double tap\n";
    std::cout << "     flick [x=X] [y=Y] [dir=DIRECTION] [distance=PX] - Quick flick\n";
    std::cout << "     swipe [x=X] [y=Y] [dir=DIRECTION] [distance=PX] [duration=MS] - Swipe gesture\n";
    std::cout << "     pan [x=X] [y=Y] [endx=X] [endy=Y] [duration=MS] - Pan gesture\n";
    std::cout << "\n   Short forms: t, dt, f, s, p\n";
    std::cout << "\n   Parameters:\n";
    std::cout << "     x, y        - Start coordinates (default: center)\n";
    std::cout << "     endx, endy  - End coordinates (for pan)\n";
    std::cout << "     direction   - up, down, left, right (for flick/swipe)\n";
    std::cout << "     distance    - Distance in pixels (default: 200)\n";
    std::cout << "     duration    - Duration in milliseconds (default: 100)\n";
    std::cout << "\n   Examples:\n";
    std::cout << "     tap x=100 y=200\n";
    std::cout << "     swipe dir=right distance=300 duration=500\n";
    std::cout << "     pan endx=800 endy=600 duration=1000\n";
    std::cout << "\nType 'random' to switch back to random mode\n";
    std::cout << "Other commands: help, quit, exit\n\n";
}

GestureType getGestureTypeFromNumber(int number) {
    switch (number) {
        case 1: return GestureType::Tap;
        case 2: return GestureType::DoubleTap;
        case 3: return GestureType::Flick;
        case 4: return GestureType::Swipe;
        case 5: return GestureType::Pan;
        default: return GestureType::Tap; // fallback
    }
}

// Interactive terminal application for gesture testing
int main(){
    Config cfg;
    cfg.screenWidth = 2560;
    cfg.screenHeight = 1440;
    cfg.inputRateHz = 30.0;  // 30fps input rate for upsampling testing
    cfg.outputRateHz = 120.0;
    cfg.maxExtrapolationMs = 50.0;
    cfg.touchTimeoutMs = 200.0;
    cfg.deviceName = "TouchDV Gesture Tester";

    VirtualTouchDevice vdev(cfg);

    // Configure OneEuro smoother for smooth gestures
    SmoothingConfig smoothConfig;
    smoothConfig.oneEuroFreq = 120.0;
    smoothConfig.oneEuroMinCutoff = 1.0;
    smoothConfig.oneEuroBeta = 0.007;
    smoothConfig.oneEuroDCutoff = 1.0;
    vdev.setSmoothingType(SmoothingType::OneEuro, smoothConfig);

    if (!vdev.start()) {
        std::cerr << "Failed to start virtual touch device\n";
        return 1;
    }

    std::cout << "TouchDV Gesture Tester started successfully!\n";
    std::cout << "Screen resolution: " << cfg.screenWidth << "x" << cfg.screenHeight << "\n";

    GestureGenerator generator(vdev, cfg);
    RandomParameterGenerator randomGen(cfg);

    bool randomMode = true; // Start in random mode
    printGestureMenu();

    std::string input;
    while (true) {
        if (randomMode) {
            std::cout << "touchdv[random]> ";
        } else {
            std::cout << "touchdv[manual]> ";
        }
        std::getline(std::cin, input);

        if (input.empty()) continue;

        // Trim whitespace
        input.erase(0, input.find_first_not_of(" \t"));
        input.erase(input.find_last_not_of(" \t") + 1);

        if (input == "quit" || input == "exit" || input == "q") {
            break;
        }

        if (input == "help" || input == "h") {
            printHelp();
            continue;
        }

        if (input == "manual" || input == "m") {
            randomMode = false;
            std::cout << "Switched to manual mode. Type gesture commands with parameters.\n";
            continue;
        }

        if (input == "random" || input == "r") {
            randomMode = true;
            printGestureMenu();
            continue;
        }

        if (randomMode) {
            // Handle random mode - expect numbers 1-5
            try {
                int gestureNum = std::stoi(input);
                if (gestureNum >= 1 && gestureNum <= 5) {
                    GestureType gestureType = getGestureTypeFromNumber(gestureNum);
                    GestureParams params = randomGen.generateRandomParams(gestureType);

                    // Print what we're going to do
                    std::cout << "Generated random ";
                    switch (gestureType) {
                        case GestureType::Tap:
                            std::cout << "tap";
                            break;
                        case GestureType::DoubleTap:
                            std::cout << "double tap";
                            break;
                        case GestureType::Flick:
                            std::cout << "flick";
                            break;
                        case GestureType::Swipe:
                            std::cout << "swipe";
                            break;
                        case GestureType::Pan:
                            std::cout << "pan";
                            break;
                    }
                    std::cout << " with random parameters...\n";

                    // Execute the gesture
                    switch (gestureType) {
                        case GestureType::Tap:
                            generator.performTap(params);
                            break;
                        case GestureType::DoubleTap:
                            generator.performDoubleTap(params);
                            break;
                        case GestureType::Flick:
                            generator.performFlick(params);
                            break;
                        case GestureType::Swipe:
                            generator.performSwipe(params);
                            break;
                        case GestureType::Pan:
                            generator.performPan(params);
                            break;
                    }

                    // Small delay after gesture completion
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                } else {
                    std::cout << "Invalid gesture number. Enter 1-5, or type 'help' for more options.\n";
                }
            } catch (const std::exception&) {
                std::cout << "Invalid input. Enter a number 1-5, or type 'help' for more options.\n";
            }
        } else {
            // Handle manual mode - use existing command parser
            GestureType gestureType;
            GestureParams params;

            // Set default position to screen center
            params.x = cfg.screenWidth / 2.0f;
            params.y = cfg.screenHeight / 2.0f;

            if (CommandParser::parseGestureCommand(input, gestureType, params)) {
                try {
                    switch (gestureType) {
                        case GestureType::Tap:
                            generator.performTap(params);
                            break;
                        case GestureType::DoubleTap:
                            generator.performDoubleTap(params);
                            break;
                        case GestureType::Flick:
                            generator.performFlick(params);
                            break;
                        case GestureType::Swipe:
                            generator.performSwipe(params);
                            break;
                        case GestureType::Pan:
                            generator.performPan(params);
                            break;
                    }

                    // Small delay after gesture completion
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                } catch (const std::exception& e) {
                    std::cerr << "Error executing gesture: " << e.what() << "\n";
                }
            } else {
                std::cout << "Unknown command. Type 'help' for usage information.\n";
            }
        }
    }

    std::cout << "Stopping virtual touch device...\n";
    vdev.stop();
    std::cout << "Goodbye!\n";
    return 0;
}
