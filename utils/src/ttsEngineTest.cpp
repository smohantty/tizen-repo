#include "ttsEngine.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

using namespace utils;

int main() {
    std::cout << "TTS Engine Example\n";
    std::cout << "==================\n\n";

    // Configure the TTS engine
    TtsConfig config;
    config.apiUrl = "https://your-tts-api.com/synthesize";
    config.format = AudioFormat::Binary; // or AudioFormat::Base64
    config.headers["Authorization"] = "Bearer YOUR_API_KEY";
    config.headers["Accept"] = "audio/wav";
    config.timeoutMs = 30000;

    // Create TTS engine
    TtsEngine engine(config);

    // Output file for testing
    std::ofstream audioFile("output.wav", std::ios::binary);
    size_t totalBytes = 0;
    bool hasError = false;

    // Define callback for receiving audio chunks
    auto audioCallback = [&](const std::vector<uint8_t>& audioData, TtsError error) {
        if (error != TtsError::None) {
            if (error == TtsError::NetworkError) {
                std::cerr << "Network error occurred\n";
            } else if (error == TtsError::InvalidResponse) {
                std::cerr << "Invalid response from server\n";
            } else if (error == TtsError::DecodingError) {
                std::cerr << "Error decoding audio data\n";
            } else if (error == TtsError::RequestCancelled) {
                std::cerr << "Request was cancelled\n";
            }
            hasError = true;
            return;
        }

        if (audioData.empty()) {
            std::cout << "\nSynthesis complete! Total bytes received: " << totalBytes << "\n";
            return;
        }

        // Write audio data to file
        audioFile.write(reinterpret_cast<const char*>(audioData.data()), audioData.size());
        totalBytes += audioData.size();

        std::cout << "Received audio chunk: " << audioData.size() << " bytes "
                  << "(total: " << totalBytes << " bytes)\r" << std::flush;

        // Here you would send the audio data to your audio player
        // For example: audioPlayer->play(audioData);
    };

    // Synthesize text to speech
    std::cout << "Synthesizing text...\n";
    std::string text = "Hello, this is a test of the text to speech engine.";

    // Optional additional parameters (e.g., voice, language, speed)
    std::map<std::string, std::string> params;
    params["voice"] = "en-US-Neural";
    params["speed"] = "1.0";
    params["pitch"] = "1.0";

    TtsError result = engine.synthesize(text, audioCallback, params);

    audioFile.close();

    if (result == TtsError::None && !hasError) {
        std::cout << "\nSuccess! Audio saved to output.wav\n";
        return 0;
    } else {
        std::cerr << "\nSynthesis failed!\n";
        return 1;
    }
}

