#include <iostream>
#include <vector>
#include "ssl/ssl.hpp"
#include "WakeupWordDetector.h"

int main() {
    std::cout << "ssl version: " << ssl::version() << "\n";
    std::cout << (ssl::initialize() ? "init ok" : "init failed") << "\n";

    // Test wakeword detector
    wakeword::WakeupWordDetector detector("/path/to/model.bin");

    // Simulate some audio data
    std::vector<short> audioData = {1000, 2000, 30000, 4000, 5000};
    bool detected = detector.processAudioBuffer(audioData);

    std::cout << "Wakeword detected: " << (detected ? "yes" : "no") << "\n";
    std::cout << "Current detection state: " << (detector.isWakeupWordDetected() ? "detected" : "not detected") << "\n";

    return 0;
}


