/**
 * @file simple_vad_example.cpp
 * @brief Simple example demonstrating the new VoiceActivityDetector API
 */

#include "VoiceActivityDetector.h"
#include <iostream>
#include <vector>
#include <random>

using namespace vad;

int main() {
    std::cout << "=== VoiceActivityDetector Refactored API Demo ===\n\n";

    try {
        // Create VAD instance with mock TensorFlow Lite model
        // Frame size is fixed at 160 samples (10ms) as required by the model
        VoiceActivityDetector detector("mock_model.tflite", 16000);

        // Set up callback to handle speech events with three states
        detector.setSpeechEventCallback([](vad::SpeechState speechState, const std::vector<short>& audioBuffer, uint64_t timestamp) {
            switch (speechState) {
                case vad::SpeechState::START:
                    std::cout << "🎤 [" << timestamp << "ms] Speech STARTED\n";
                    std::cout << "   📋 Preroll buffer size: " << audioBuffer.size() << " samples\n";
                    std::cout << "   🚀 Initializing ASR with preroll data...\n";
                    break;

                case vad::SpeechState::CONTINUE:
                    std::cout << "🔄 [" << timestamp << "ms] Speech CONTINUING\n";
                    std::cout << "   📊 Streaming " << audioBuffer.size() << " samples to ASR...\n";
                    break;

                case vad::SpeechState::END:
                    std::cout << "🔇 [" << timestamp << "ms] Speech ENDED\n";
                    std::cout << "   ✅ Finalizing ASR processing...\n";
                    break;
            }
        });

        // Configure detection parameters
        detector.setSpeechThreshold(0.3f);        // Lower threshold for demo
        detector.setMinSpeechDuration(100);       // 100ms minimum speech
        detector.setMinSilenceDuration(200);      // 200ms minimum silence
        // Note: Preroll buffer is fixed at 500ms internally

        std::cout << "Processing simulated audio streams...\n\n";

        // Simulate random audio data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<short> noise(-100, 100);
        std::uniform_int_distribution<short> speech(-5000, 5000);

        // Process sequence: silence -> speech -> silence -> speech -> silence
        std::vector<std::pair<std::string, bool>> sequence = {
            {"silence", false},
            {"speech", true},
            {"silence", false},
            {"speech", true},
            {"silence", false}
        };

        for (const auto& [type, isSpeech] : sequence) {
            std::cout << "Processing " << type << "...\n";

            // Process multiple chunks for each type
            for (int chunk = 0; chunk < 50; ++chunk) { // More chunks since frames are smaller
                std::vector<short> audioData(160); // 10ms at 16kHz (TensorFlow Lite model requirement)

                // Generate appropriate audio data
                for (auto& sample : audioData) {
                    sample = isSpeech ? speech(gen) : noise(gen);
                }

                // Process audio using the new API
                detector.process(std::move(audioData));

                // Check current state
                if (detector.isSpeechActive()) {
                    std::cout << "  🟢 Speech active\n";
                } else {
                    std::cout << "  🔴 No speech\n";
                }
            }

            std::cout << "\n";
        }

        std::cout << "Demo completed successfully!\n";
        std::cout << "\nNew API Features:\n";
        std::cout << "✅ Simple process(audiobuff) API\n";
        std::cout << "✅ TensorFlow Lite integration (mock)\n";
        std::cout << "✅ Three-state callbacks (START/CONTINUE/END)\n";
        std::cout << "✅ Preroll buffer for ASR initialization\n";
        std::cout << "✅ Streaming audio for continuous ASR\n";
        std::cout << "✅ Probability-based detection\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
