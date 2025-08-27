#include "VoiceActivityDetector.h"
#include <iostream>
#include <vector>

// Your ASR module interface
class ASRModule {
public:
    std::string processAudio(const std::vector<short>& buffer, bool isEndOfSpeech) {
        if (isEndOfSpeech) {
            std::cout << "ASR: End of speech - generating result\n";
            return "FINAL_TRANSCRIPTION_RESULT";
        } else {
            std::cout << "ASR: Processing speech chunk (" << buffer.size() << " samples)\n";
            return "";
        }
    }

    void reset() {
        std::cout << "ASR: Session reset\n";
    }
};

// Clean VAD-ASR integration using callbacks
class CallbackASRIntegration {
private:
    vad::VoiceActivityDetector mVAD;
    ASRModule mASR;
    bool mASRActive;

public:
    CallbackASRIntegration() : mASRActive(false) {
        // Configure VAD
        mVAD.setEnergyThreshold(5000.0);
        mVAD.setMinSpeechDuration(200);
        mVAD.setMinSilenceDuration(500);

        // Set up callback for automatic ASR control
        mVAD.setSpeechEventCallback([this](bool isSpeechActive, uint64_t timestamp) {
            if (isSpeechActive) {
                // Speech started - automatically initialize ASR
                std::cout << "🎤 [" << timestamp << "ms] Speech started - Auto-starting ASR\n";
                mASR.reset();
                mASRActive = true;
            } else {
                // Speech ended - automatically finalize ASR
                std::cout << "🔇 [" << timestamp << "ms] Speech ended - Auto-finalizing ASR\n";
                if (mASRActive) {
                    std::string result = mASR.processAudio({}, true);
                    if (!result.empty()) {
                        std::cout << "📝 RESULT: \"" << result << "\"\n";
                    }
                    mASRActive = false;
                }
            }
        });
    }

    // Super simple processing - just feed audio, callbacks handle the rest!
    void processAudioBuffer(const std::vector<short>& buffer) {
        // Process through VAD (callbacks automatically handle ASR lifecycle)
        mVAD.processAudioBuffer(buffer);

        // Only send to ASR if speech is active (automatic filtering)
        if (mVAD.isSpeechActive() && mASRActive) {
            mASR.processAudio(buffer, false);
        }
    }
};

// Comparison: Manual vs Callback approach
void demonstrateCallbackAdvantage() {
    std::cout << "=== CALLBACK-BASED ASR INTEGRATION ===\n\n";

    CallbackASRIntegration integration;

    // Your main loop becomes super simple:
    std::vector<std::vector<short>> audioStream = {
        std::vector<short>(1600, 100),     // Silence
        std::vector<short>(1600, 100),     // Silence
        std::vector<short>(1600, 15000),   // Speech starts (callback triggers)
        std::vector<short>(1600, 18000),   // Speech continues (auto-sent to ASR)
        std::vector<short>(1600, 16000),   // Speech continues
        std::vector<short>(1600, 100),     // Silence starts
        std::vector<short>(1600, 100),     // Silence continues
        std::vector<short>(1600, 100),     // Speech ends (callback triggers)
        std::vector<short>(1600, 14000),   // New speech starts
        std::vector<short>(1600, 17000),   // Speech continues
        std::vector<short>(1600, 100),     // Silence (speech will end)
        std::vector<short>(1600, 100),     //
        std::vector<short>(1600, 100),     // Speech ends here
    };

    for (size_t i = 0; i < audioStream.size(); ++i) {
        std::cout << "Processing chunk " << i << ":\n";

        // This is ALL you need to write in your main loop!
        integration.processAudioBuffer(audioStream[i]);

        std::cout << "\n";
    }
}

// Even simpler: Pure callback-driven approach
void demonstratePureCallbackApproach() {
    std::cout << "\n=== PURE CALLBACK APPROACH ===\n\n";

    vad::VoiceActivityDetector vad;
    ASRModule asr;
    bool asrActive = false;

    // Configure VAD
    vad.setEnergyThreshold(5000.0);
    vad.setMinSpeechDuration(200);
    vad.setMinSilenceDuration(500);

    // Single callback handles everything
    vad.setSpeechEventCallback([&](bool isSpeechActive, uint64_t timestamp) {
        if (isSpeechActive) {
            std::cout << "🎤 Speech detected -> Starting ASR\n";
            asr.reset();
            asrActive = true;
        } else {
            std::cout << "🔇 Speech ended -> Finalizing ASR\n";
            if (asrActive) {
                std::string result = asr.processAudio({}, true);
                std::cout << "📝 Final result: \"" << result << "\"\n";
                asrActive = false;
            }
        }
    });

    // Your audio processing loop becomes trivial:
    auto processBuffer = [&](const std::vector<short>& buffer) {
        vad.processAudioBuffer(buffer);  // Callbacks handle ASR lifecycle

        // Only send audio during active speech
        if (vad.isSpeechActive() && asrActive) {
            asr.processAudio(buffer, false);
        }
    };

    // Simulate audio processing
    std::vector<std::vector<short>> testAudio = {
        std::vector<short>(1600, 150),    // Silence
        std::vector<short>(1600, 16000),  // Speech
        std::vector<short>(1600, 18000),  // Speech
        std::vector<short>(1600, 150),    // Silence
        std::vector<short>(1600, 100),    // Silence (triggers end)
    };

    for (const auto& chunk : testAudio) {
        processBuffer(chunk);
    }
}

int main() {
    std::cout << "Callback-Based VAD-ASR Integration\n";
    std::cout << "==================================\n\n";

    demonstrateCallbackAdvantage();
    demonstratePureCallbackApproach();

    std::cout << "\n🎯 Callback Benefits for ASR Integration:\n";
    std::cout << "✓ Automatic ASR lifecycle management\n";
    std::cout << "✓ No manual state checking required\n";
    std::cout << "✓ Event-driven architecture\n";
    std::cout << "✓ Cleaner, more maintainable code\n";
    std::cout << "✓ Precise timing information\n";
    std::cout << "✓ Easier to add logging/monitoring\n";

    return 0;
}
