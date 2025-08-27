#include "VoiceActivityDetector.h"
#include <iostream>
#include <vector>

// Your ASR module interface (replace with actual implementation)
class YourASRModule {
public:
    void processAudio(const std::vector<short>& buffer, bool isEndOfSpeech) {
        if (isEndOfSpeech) {
            std::cout << "ASR: End of speech detected, generating final result\n";
        } else {
            std::cout << "ASR: Processing speech buffer (" << buffer.size() << " samples)\n";
        }
    }

    void reset() {
        std::cout << "ASR: Session reset\n";
    }
};

// Simple VAD-filtered ASR processing
void processAudioWithVADFilter() {
    vad::VoiceActivityDetector vad;
    YourASRModule asr;

    // Configure VAD (tune these for your environment)
    vad.setEnergyThreshold(5000.0);       // Adjust for your audio level
    vad.setMinSpeechDuration(200);        // 200ms minimum speech
    vad.setMinSilenceDuration(500);       // 500ms silence to end speech

    bool asrSessionActive = false;

    // Your main audio processing loop
    auto processBuffer = [&](const std::vector<short>& audioBuffer) {
        // Process buffer through VAD
        bool vadStateChanged = vad.processAudioBuffer(audioBuffer);
        bool speechActive = vad.isSpeechActive();

        if (vadStateChanged) {
            if (speechActive) {
                // Speech started - initialize ASR
                std::cout << "🎤 Speech detected - Starting ASR\n";
                asr.reset();
                asrSessionActive = true;
            } else {
                // Speech ended - finalize ASR
                std::cout << "🔇 Speech ended - Sending end-of-speech to ASR\n";
                if (asrSessionActive) {
                    // Send empty buffer with end-of-speech flag
                    asr.processAudio(std::vector<short>(), true);
                    asrSessionActive = false;
                }
            }
        }

        // Only send audio to ASR during active speech
        if (speechActive && asrSessionActive) {
            asr.processAudio(audioBuffer, false);
        }
        // Otherwise, buffer is filtered out (not sent to ASR)
    };

    // Example: simulate incoming audio buffers
    std::cout << "Simulating audio stream processing...\n\n";

    // Silence
    processBuffer(std::vector<short>(1600, 100));
    processBuffer(std::vector<short>(1600, 150));

    // Speech
    processBuffer(std::vector<short>(1600, 15000));  // Speech starts here
    processBuffer(std::vector<short>(1600, 18000));  // Continue speech
    processBuffer(std::vector<short>(1600, 16000));  // Continue speech

    // Silence (will trigger end-of-speech)
    processBuffer(std::vector<short>(1600, 120));
    processBuffer(std::vector<short>(1600, 100));
    processBuffer(std::vector<short>(1600, 110));    // Speech ends here

    // More silence (no ASR processing)
    processBuffer(std::vector<short>(1600, 100));

    std::cout << "\nKey points:\n";
    std::cout << "- Only speech buffers are sent to ASR\n";
    std::cout << "- ASR gets clear end-of-speech signals\n";
    std::cout << "- Silence/noise is filtered out automatically\n";
}

int main() {
    std::cout << "Simple VAD Filter for ASR\n";
    std::cout << "=========================\n\n";

    processAudioWithVADFilter();

    return 0;
}
