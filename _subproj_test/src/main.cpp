#include <iostream>
#include <vector>
#include "ssl/ssl.hpp"
#include "VoiceActivityDetector.h"

int main() {
    std::cout << "ssl version: " << ssl::version() << "\n";
    std::cout << (ssl::initialize() ? "init ok" : "init failed") << "\n";

    // Wakeword test disabled due to missing library file
    std::cout << "Wakeword test skipped\n";

    // Test VAD
    vad::VoiceActivityDetector vadDetector;
    vadDetector.setSpeechEventCallback([](bool isSpeechActive, uint64_t timestamp) {
        std::cout << "VAD: Speech " << (isSpeechActive ? "started" : "ended")
                  << " at " << timestamp << "ms\n";
    });

    std::vector<short> speechData = {10000, 15000, 20000, 18000, 12000};
    bool vadStateChanged = vadDetector.processAudioBuffer(speechData);
    std::cout << "VAD state changed: " << (vadStateChanged ? "yes" : "no") << "\n";
    std::cout << "Speech active: " << (vadDetector.isSpeechActive() ? "yes" : "no") << "\n";

    return 0;
}


