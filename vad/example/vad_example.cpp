#include "VoiceActivityDetector.h"
#include <iostream>
#include <vector>
#include <random>
#include <thread>
#include <chrono>
#include <cmath>

namespace {
    // Helper function to generate sine wave audio samples
    std::vector<short> generateSineWave(double frequency, double amplitude,
                                       double durationSeconds, int sampleRate) {
        size_t numSamples = static_cast<size_t>(durationSeconds * sampleRate);
        std::vector<short> samples(numSamples);

        for (size_t i = 0; i < numSamples; ++i) {
            double time = static_cast<double>(i) / sampleRate;
            double value = amplitude * std::sin(2.0 * M_PI * frequency * time);
            samples[i] = static_cast<short>(value);
        }

        return samples;
    }

    // Helper function to generate noise
    std::vector<short> generateNoise(double amplitude, double durationSeconds, int sampleRate) {
        size_t numSamples = static_cast<size_t>(durationSeconds * sampleRate);
        std::vector<short> samples(numSamples);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-amplitude, amplitude);

        for (size_t i = 0; i < numSamples; ++i) {
            samples[i] = static_cast<short>(dis(gen));
        }

        return samples;
    }

    // Helper function to generate silence
    std::vector<short> generateSilence(double durationSeconds, int sampleRate) {
        size_t numSamples = static_cast<size_t>(durationSeconds * sampleRate);
        return std::vector<short>(numSamples, 0);
    }
}

int main() {
    std::cout << "Voice Activity Detection Example\n";
    std::cout << "================================\n\n";

    // Create VAD instance with default parameters
    vad::VoiceActivityDetector detector(16000, 1600); // 16kHz, 100ms frames

    // Set up event callback
    detector.setSpeechEventCallback([](bool isSpeechActive, uint64_t timestamp) {
        if (isSpeechActive) {
            std::cout << "[" << timestamp << "ms] Speech STARTED\n";
        } else {
            std::cout << "[" << timestamp << "ms] Speech ENDED\n";
        }
    });

    // Configure detection parameters
    detector.setEnergyThreshold(5000.0);    // Lower threshold for demo
    detector.setMinSpeechDuration(150);     // 150ms minimum speech
    detector.setMinSilenceDuration(300);    // 300ms minimum silence

    std::cout << "Processing audio sequence:\n";
    std::cout << "1. Silence (1s)\n";
    std::cout << "2. Speech simulation (2s)\n";
    std::cout << "3. Silence (1s)\n";
    std::cout << "4. Speech simulation (1s)\n";
    std::cout << "5. Silence (1s)\n\n";

    // Simulate audio processing with different types of audio

    // 1. Initial silence (1 second)
    auto silence1 = generateSilence(1.0, 16000);
    std::cout << "Processing silence...\n";
    detector.processAudioBuffer(silence1);

    // 2. Speech simulation using sine wave + noise (2 seconds)
    auto speech1 = generateSineWave(440.0, 15000.0, 2.0, 16000); // A4 note
    auto noise1 = generateNoise(2000.0, 2.0, 16000);

    // Mix sine wave with noise to simulate speech
    for (size_t i = 0; i < speech1.size() && i < noise1.size(); ++i) {
        speech1[i] = static_cast<short>((speech1[i] + noise1[i]) * 0.7);
    }

    std::cout << "Processing speech...\n";
    detector.processAudioBuffer(speech1);

    // 3. Silence (1 second)
    auto silence2 = generateSilence(1.0, 16000);
    std::cout << "Processing silence...\n";
    detector.processAudioBuffer(silence2);

    // 4. Another speech simulation (1 second)
    auto speech2 = generateSineWave(880.0, 12000.0, 1.0, 16000); // A5 note
    auto noise2 = generateNoise(3000.0, 1.0, 16000);

    for (size_t i = 0; i < speech2.size() && i < noise2.size(); ++i) {
        speech2[i] = static_cast<short>((speech2[i] + noise2[i]) * 0.8);
    }

    std::cout << "Processing more speech...\n";
    detector.processAudioBuffer(speech2);

    // 5. Final silence (1 second)
    auto silence3 = generateSilence(1.0, 16000);
    std::cout << "Processing final silence...\n";
    detector.processAudioBuffer(silence3);

    std::cout << "\nFinal state:\n";
    std::cout << "Speech active: " << (detector.isSpeechActive() ? "YES" : "NO") << "\n";
    std::cout << "Last state change: " << detector.getLastStateChangeTimestamp() << "ms\n";

    std::cout << "\n--- Testing Real-time Processing ---\n";
    std::cout << "Simulating real-time audio with small buffers...\n\n";

    // Reset detector for real-time simulation
    detector.reset();

    // Simulate real-time processing with smaller buffers (10ms chunks)
    constexpr int totalChunks = 500;  // 5 seconds total

    for (int chunk = 0; chunk < totalChunks; ++chunk) {
        std::vector<short> buffer;

        // Create different audio patterns based on time
        double time = chunk * 10.0; // Time in milliseconds

        if (time < 500 || (time >= 1500 && time < 2000) || time >= 3500) {
            // Silence periods
            buffer = generateSilence(0.01, 16000); // 10ms silence
        } else {
            // Speech periods with varying intensity
            double frequency = 440.0 + (time / 10.0); // Varying frequency
            double amplitude = 8000.0 + 4000.0 * std::sin(time / 200.0);
            buffer = generateSineWave(frequency, amplitude, 0.01, 16000);

            // Add some noise
            auto noise = generateNoise(1000.0, 0.01, 16000);
            for (size_t i = 0; i < buffer.size() && i < noise.size(); ++i) {
                buffer[i] = static_cast<short>((buffer[i] + noise[i]) * 0.8);
            }
        }

        // Process the chunk
        detector.processAudioBuffer(buffer);

        // Print progress every 100ms
        if (chunk % 10 == 0) {
            std::cout << "[" << time << "ms] "
                      << (detector.isSpeechActive() ? "SPEECH" : "SILENCE")
                      << "\n";
        }

        // Simulate real-time delay
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "\nExample completed successfully!\n";
    std::cout << "The VAD system detected speech start/end events during the simulation.\n";

    return 0;
}
