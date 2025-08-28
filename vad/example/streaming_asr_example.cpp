/**
 * @file streaming_asr_example.cpp
 * @brief Example demonstrating VAD integration with streaming ASR using three-state callbacks
 */

#include "VoiceActivityDetector.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

using namespace vad;

/**
 * @brief Mock ASR class to demonstrate integration
 */
class MockStreamingASR {
private:
    bool mIsInitialized = false;
    std::vector<short> mAccumulatedAudio;

public:
    void initialize(const std::vector<short>& prerollBuffer) {
        std::cout << "   🚀 ASR: Initializing with " << prerollBuffer.size() << " preroll samples\n";
        mAccumulatedAudio = prerollBuffer;
        mIsInitialized = true;
    }

    void addAudioChunk(const std::vector<short>& audioChunk) {
        if (!mIsInitialized) {
            std::cout << "   ⚠️  ASR: Not initialized!\n";
            return;
        }

        mAccumulatedAudio.insert(mAccumulatedAudio.end(), audioChunk.begin(), audioChunk.end());
        std::cout << "   📊 ASR: Processing " << audioChunk.size() << " samples (total: "
                  << mAccumulatedAudio.size() << ")\n";

        // Mock: simulate partial results every few chunks
        if (mAccumulatedAudio.size() > 8000) { // ~500ms at 16kHz
            std::cout << "   💬 ASR: Partial result: \"Hello there...\"\n";
        }
    }

    std::string finalize() {
        if (!mIsInitialized) {
            return "";
        }

        std::cout << "   🎯 ASR: Finalizing with " << mAccumulatedAudio.size() << " total samples\n";

        // Mock: generate final result based on audio length
        std::string result;
        if (mAccumulatedAudio.size() > 16000) { // ~1 second
            result = "Hello there, how are you doing today?";
        } else if (mAccumulatedAudio.size() > 8000) { // ~0.5 seconds
            result = "Hello there";
        } else {
            result = "Hi";
        }

        mIsInitialized = false;
        mAccumulatedAudio.clear();
        return result;
    }

    bool isInitialized() const { return mIsInitialized; }
};

int main() {
    std::cout << "=== Streaming ASR Integration with Three-State VAD ===\n\n";

    try {
        // Create VAD and ASR instances
        // Frame size is fixed at 160 samples (10ms) as required by TensorFlow Lite model
        VoiceActivityDetector detector("mock_model.tflite", 16000);
        MockStreamingASR asr;

        // Set up three-state callback for ASR integration
        detector.setSpeechEventCallback([&asr](SpeechState speechState, const std::vector<short>& audioBuffer, uint64_t timestamp) {
            switch (speechState) {
                case SpeechState::START:
                    std::cout << "🎤 [" << timestamp << "ms] Speech STARTED\n";
                    asr.initialize(audioBuffer);
                    break;

                case SpeechState::CONTINUE:
                    std::cout << "🔄 [" << timestamp << "ms] Speech CONTINUING\n";
                    asr.addAudioChunk(audioBuffer);
                    break;

                case SpeechState::END:
                    std::cout << "🔇 [" << timestamp << "ms] Speech ENDED\n";
                    if (asr.isInitialized()) {
                        std::string result = asr.finalize();
                        std::cout << "   ✅ Final ASR Result: \"" << result << "\"\n";
                    }
                    break;
            }
        });

        // Configure for more sensitive detection to demonstrate callbacks
        detector.setSpeechThreshold(0.1f);        // Very low threshold for demo
        detector.setMinSpeechDuration(50);        // 50ms minimum speech
        detector.setMinSilenceDuration(100);      // 100ms minimum silence
        // Note: Preroll buffer is fixed at 500ms internally

        std::cout << "🎯 Processing audio sequence to demonstrate ASR integration...\n\n";

        // Simulate a conversation with multiple utterances
        std::vector<std::tuple<std::string, bool, int>> audioSequence = {
            {"background noise", false, 3},
            {"first utterance", true, 6},
            {"pause", false, 2},
            {"second utterance", true, 8},
            {"final silence", false, 3}
        };

        for (const auto& [description, isSpeech, chunkCount] : audioSequence) {
            std::cout << "📡 " << description << "...\n";

            for (int i = 0; i < chunkCount * 10; ++i) { // More iterations since frames are smaller
                // Generate mock audio data
                std::vector<short> audioData(160); // 10ms chunks (TensorFlow Lite model requirement)

                if (isSpeech) {
                    // Generate higher amplitude "speech" signals
                    for (size_t j = 0; j < audioData.size(); ++j) {
                        audioData[j] = static_cast<short>((j % 1000) * 3); // Simple pattern
                    }
                } else {
                    // Generate low amplitude "noise"
                    for (size_t j = 0; j < audioData.size(); ++j) {
                        audioData[j] = static_cast<short>(j % 50); // Low amplitude
                    }
                }

                // Process with VAD
                detector.process(std::move(audioData));

                // Show current detection state
                std::cout << "   " << (detector.isSpeechActive() ? "🟢" : "🔴") << " VAD State\n";
            }
            std::cout << "\n";
        }

        std::cout << "🎉 Demo completed!\n\n";
        std::cout << "💡 Integration Pattern:\n";
        std::cout << "   START    → Initialize ASR with preroll buffer\n";
        std::cout << "   CONTINUE → Stream audio chunks to ASR\n";
        std::cout << "   END      → Finalize ASR and get result\n\n";
        std::cout << "🔧 Perfect for real-time speech recognition systems!\n";

    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
