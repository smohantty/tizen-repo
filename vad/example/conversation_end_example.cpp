#include "VoiceActivityDetector.h"
#include <iostream>
#include <vector>

int main() {
    try {
        // Create VAD instance
        vad::VoiceActivityDetector detector("mock_model.tflite", 16000);

        // Configure conversation timeout (default is 2000ms, setting to 1500ms for demo)
        detector.setConversationTimeout(1500);

        // Set up callback to handle speech events
        detector.setSpeechEventCallback([](vad::SpeechState state, const std::vector<short>& buffer, uint64_t timestamp) {
            switch (state) {
                case vad::SpeechState::START:
                    std::cout << "[" << timestamp << "ms] CONVERSATION STARTED - Speech detected, ASR initialized with "
                              << buffer.size() << " preroll samples" << std::endl;
                    break;

                case vad::SpeechState::CONTINUE:
                    std::cout << "[" << timestamp << "ms] Speech continuing - " << buffer.size() << " samples" << std::endl;
                    break;

                case vad::SpeechState::END:
                    std::cout << "[" << timestamp << "ms] SPEECH SEGMENT ENDED - Finalizing current speech segment" << std::endl;
                    break;

                case vad::SpeechState::CONVERSATION_END:
                    std::cout << "[" << timestamp << "ms] CONVERSATION ENDED - Entire conversation finished after timeout" << std::endl;
                    break;
            }
        });

        std::cout << "Voice Activity Detector with Conversation End Detection" << std::endl;
        std::cout << "======================================================" << std::endl;
        std::cout << "Demonstrating conversation flow:" << std::endl;
        std::cout << "1. Speech START -> CONTINUE -> END (first speech segment)" << std::endl;
        std::cout << "2. Speech START -> CONTINUE -> END (second speech segment)" << std::endl;
        std::cout << "3. After 1500ms timeout -> CONVERSATION_END" << std::endl;
        std::cout << std::endl;

        // Simulate audio processing with conversation flow
        // Note: This uses the mock implementation which is energy-based

        // 1. Simulate first speech segment (high energy audio)
        for (int i = 0; i < 20; ++i) {
            std::vector<short> audioData(160);
            // Generate high energy audio (simulates speech)
            for (size_t j = 0; j < audioData.size(); ++j) {
                audioData[j] = static_cast<short>(5000 + (rand() % 2000));
            }
            detector.process(audioData);
        }

        // 2. Simulate silence between speech segments (low energy)
        for (int i = 0; i < 50; ++i) {
            std::vector<short> audioData(160);
            // Generate low energy audio (simulates silence)
            for (size_t j = 0; j < audioData.size(); ++j) {
                audioData[j] = static_cast<short>(rand() % 100);
            }
            detector.process(audioData);
        }

        // 3. Simulate second speech segment (high energy audio)
        for (int i = 0; i < 15; ++i) {
            std::vector<short> audioData(160);
            // Generate high energy audio (simulates speech)
            for (size_t j = 0; j < audioData.size(); ++j) {
                audioData[j] = static_cast<short>(4500 + (rand() % 2000));
            }
            detector.process(audioData);
        }

        // 4. Simulate extended silence to trigger conversation end (low energy for conversation timeout)
        for (int i = 0; i < 200; ++i) {  // Process enough frames to exceed conversation timeout
            std::vector<short> audioData(160);
            // Generate low energy audio (simulates extended silence)
            for (size_t j = 0; j < audioData.size(); ++j) {
                audioData[j] = static_cast<short>(rand() % 50);
            }
            detector.process(audioData);
        }

        std::cout << std::endl;
        std::cout << "Demo completed. Notice the difference between:" << std::endl;
        std::cout << "- Speech END: Individual speech segments ending" << std::endl;
        std::cout << "- CONVERSATION_END: Entire conversation ending after timeout" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
