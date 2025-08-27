#include "VoiceActivityDetector.h"
#include <iostream>
#include <vector>
#include <queue>
#include <string>

// Mock ASR module interface (replace with your actual ASR)
class ASRModule {
public:
    // Your ASR interface - takes audio buffer and end-of-speech flag
    std::string processAudio(const std::vector<short>& buffer, bool isEndOfSpeech) {
        if (isEndOfSpeech) {
            std::cout << "ASR: Processing final speech segment, returning result...\n";
            return "RECOGNIZED_TEXT_RESULT";
        } else {
            std::cout << "ASR: Processing speech chunk (" << buffer.size() << " samples)\n";
            return ""; // Partial processing, no result yet
        }
    }

    void reset() {
        std::cout << "ASR: Reset for new utterance\n";
    }
};

// VAD-ASR Integration Class
class VADFilteredASR {
private:
    vad::VoiceActivityDetector mVAD;
    ASRModule mASR;
    std::queue<std::vector<short>> mSpeechBufferQueue;
    bool mCurrentlySendingToASR;

public:
    VADFilteredASR() : mCurrentlySendingToASR(false) {
        // Configure VAD for speech filtering
        mVAD.setEnergyThreshold(5000.0);      // Adjust based on your audio
        mVAD.setMinSpeechDuration(200);       // 200ms minimum speech
        mVAD.setMinSilenceDuration(500);      // 500ms silence to end speech
    }

    // Main method: process audio and filter for ASR
    std::string processAudioBuffer(const std::vector<short>& buffer) {
        bool stateChanged = mVAD.processAudioBuffer(buffer);
        bool isSpeechActive = mVAD.isSpeechActive();

        // Handle speech state transitions
        if (stateChanged) {
            if (isSpeechActive) {
                // Speech just started
                handleSpeechStart();
            } else {
                // Speech just ended
                return handleSpeechEnd();
            }
        }

        // If speech is active, send buffer to ASR
        if (isSpeechActive) {
            return sendToASR(buffer, false); // Not end of speech
        }

        return ""; // No speech, no ASR processing
    }

private:
    void handleSpeechStart() {
        std::cout << "VAD: Speech started - initializing ASR\n";
        if (!mCurrentlySendingToASR) {
            mASR.reset();
            mCurrentlySendingToASR = true;
        }
    }

    std::string handleSpeechEnd() {
        std::cout << "VAD: Speech ended - finalizing ASR\n";
        if (mCurrentlySendingToASR) {
            mCurrentlySendingToASR = false;

            // Send empty buffer with end-of-speech flag to get final result
            std::vector<short> emptyBuffer;
            return sendToASR(emptyBuffer, true); // End of speech
        }
        return "";
    }

    std::string sendToASR(const std::vector<short>& buffer, bool isEndOfSpeech) {
        if (mCurrentlySendingToASR || isEndOfSpeech) {
            return mASR.processAudio(buffer, isEndOfSpeech);
        }
        return "";
    }
};

// Example usage scenarios
void demonstrateBasicFiltering() {
    std::cout << "\n=== DEMO 1: Basic VAD Filtering ===\n";

    vad::VoiceActivityDetector vad;
    ASRModule asr;

    vad.setEnergyThreshold(5000.0);
    vad.setMinSpeechDuration(150);
    vad.setMinSilenceDuration(300);

    // Simulate audio processing loop
    std::vector<std::vector<short>> audioChunks = {
        std::vector<short>(1600, 100),     // Silence
        std::vector<short>(1600, 100),     // Silence
        std::vector<short>(1600, 15000),   // Speech starts
        std::vector<short>(1600, 18000),   // Speech continues
        std::vector<short>(1600, 16000),   // Speech continues
        std::vector<short>(1600, 12000),   // Speech continues
        std::vector<short>(1600, 100),     // Silence starts
        std::vector<short>(1600, 100),     // Silence continues
        std::vector<short>(1600, 100),     // Silence continues (speech should end)
    };

    bool asrActive = false;

    for (size_t i = 0; i < audioChunks.size(); ++i) {
        bool stateChanged = vad.processAudioBuffer(audioChunks[i]);
        bool isSpeechActive = vad.isSpeechActive();

        std::cout << "Chunk " << i << ": ";

        if (stateChanged) {
            if (isSpeechActive) {
                std::cout << "Speech STARTED - Reset ASR\n";
                asr.reset();
                asrActive = true;
            } else {
                std::cout << "Speech ENDED - Finalize ASR\n";
                if (asrActive) {
                    std::string result = asr.processAudio({}, true); // End of speech
                    std::cout << "ASR Result: " << result << "\n";
                    asrActive = false;
                }
            }
        } else {
            std::cout << (isSpeechActive ? "Speech continues" : "Silence") << "\n";
        }

        // Only send to ASR if speech is active
        if (isSpeechActive && asrActive) {
            asr.processAudio(audioChunks[i], false);
        }
    }
}

void demonstrateIntegratedClass() {
    std::cout << "\n=== DEMO 2: Integrated VAD-ASR Class ===\n";

    VADFilteredASR vadASR;

    // Simulate continuous audio processing
    std::vector<std::vector<short>> audioStream = {
        std::vector<short>(1600, 100),     // Silence
        std::vector<short>(1600, 100),     // Silence
        std::vector<short>(1600, 15000),   // Speech chunk 1
        std::vector<short>(1600, 18000),   // Speech chunk 2
        std::vector<short>(1600, 16000),   // Speech chunk 3
        std::vector<short>(1600, 100),     // Silence starts
        std::vector<short>(1600, 100),     // Silence continues
        std::vector<short>(1600, 100),     // Speech ends here
        std::vector<short>(1600, 100),     // More silence
        std::vector<short>(1600, 14000),   // New speech starts
        std::vector<short>(1600, 17000),   // New speech continues
        std::vector<short>(1600, 100),     // Silence
        std::vector<short>(1600, 100),     // Speech ends
    };

    for (size_t i = 0; i < audioStream.size(); ++i) {
        std::cout << "Processing chunk " << i << ":\n";
        std::string result = vadASR.processAudioBuffer(audioStream[i]);

        if (!result.empty()) {
            std::cout << "*** FINAL ASR RESULT: " << result << " ***\n";
        }
        std::cout << "\n";
    }
}

// Real-world usage pattern
void demonstrateRealWorldUsage() {
    std::cout << "\n=== DEMO 3: Real-world Usage Pattern ===\n";

    vad::VoiceActivityDetector vad;
    ASRModule asr;

    // Configure for your specific audio characteristics
    vad.setEnergyThreshold(8000.0);       // Tune for your microphone/environment
    vad.setMinSpeechDuration(250);        // 250ms minimum speech duration
    vad.setMinSilenceDuration(800);       // 800ms silence to confirm speech end

    bool asrSessionActive = false;
    std::string accumulatedResult;

    // This would be your main audio processing loop
    auto processAudioChunk = [&](const std::vector<short>& audioChunk) -> std::string {
        bool vadStateChanged = vad.processAudioBuffer(audioChunk);
        bool speechActive = vad.isSpeechActive();

        if (vadStateChanged) {
            if (speechActive) {
                // Speech started - begin ASR session
                std::cout << "🎤 Speech detected - Starting ASR session\n";
                asr.reset();
                asrSessionActive = true;
                accumulatedResult.clear();
            } else {
                // Speech ended - finalize ASR session
                std::cout << "🔇 Speech ended - Finalizing ASR\n";
                if (asrSessionActive) {
                    std::string finalResult = asr.processAudio({}, true);
                    asrSessionActive = false;
                    return finalResult;
                }
            }
        }

        // Send audio to ASR only during active speech
        if (speechActive && asrSessionActive) {
            asr.processAudio(audioChunk, false);
        }

        return "";
    };

    // Simulate processing multiple audio chunks
    std::vector<std::vector<short>> realWorldAudio = {
        // Background noise
        std::vector<short>(1600, 200),
        std::vector<short>(1600, 150),
        // User says "Hello world"
        std::vector<short>(1600, 12000),
        std::vector<short>(1600, 15000),
        std::vector<short>(1600, 18000),
        std::vector<short>(1600, 14000),
        // Pause
        std::vector<short>(1600, 180),
        std::vector<short>(1600, 200),
        std::vector<short>(1600, 150),
        // User says "How are you"
        std::vector<short>(1600, 16000),
        std::vector<short>(1600, 19000),
        std::vector<short>(1600, 17000),
        // End silence
        std::vector<short>(1600, 180),
        std::vector<short>(1600, 150),
        std::vector<short>(1600, 200),
    };

    for (const auto& chunk : realWorldAudio) {
        std::string result = processAudioChunk(chunk);
        if (!result.empty()) {
            std::cout << "📝 TRANSCRIPTION: \"" << result << "\"\n\n";
        }
    }
}

int main() {
    std::cout << "VAD-ASR Integration Examples\n";
    std::cout << "============================\n";

    demonstrateBasicFiltering();
    demonstrateIntegratedClass();
    demonstrateRealWorldUsage();

    std::cout << "\nKey Benefits of VAD Filtering:\n";
    std::cout << "✓ Only processes audio during speech (saves compute)\n";
    std::cout << "✓ Automatically detects utterance boundaries\n";
    std::cout << "✓ Prevents ASR from processing noise/silence\n";
    std::cout << "✓ Provides clear end-of-speech signals to ASR\n";

    return 0;
}
