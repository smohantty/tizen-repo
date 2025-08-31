#include "AudioStreaming.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <random>

using namespace edgeprocessor;

// Mock transport adapter for demonstration
class MockTransportAdapter : public ITransportAdapter {
public:
    MockTransportAdapter() : mConnected(true), mReceiveCallback(nullptr) {}

    bool send(const std::string& jsonMessage) override {
        std::cout << "Sending: " << jsonMessage << std::endl;

        // Simulate processing delay
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Simulate ASR responses
        simulateAsrResponse();

        return mConnected;
    }

    void setReceiveCallback(std::function<void(const std::string&)> callback) override {
        mReceiveCallback = callback;
    }

    bool isConnected() const override {
        return mConnected;
    }

    void disconnect() {
        mConnected = false;
    }

private:
    void simulateAsrResponse() {
        if (!mReceiveCallback) return;

        static int responseCount = 0;
        responseCount++;

        // Simulate different types of responses
        switch (responseCount % 5) {
            case 0:
                // Partial result
                mReceiveCallback("{\"type\":\"partial\",\"text\":\"hello wor\",\"stability\":0.85}");
                break;
            case 1:
                // Final result
                mReceiveCallback("{\"type\":\"final\",\"text\":\"hello world\",\"confidence\":0.94}");
                break;
            case 2:
                // Latency
                mReceiveCallback("{\"type\":\"latency\",\"upstream_ms\":42,\"e2e_ms\":120}");
                break;
            case 3:
                // Status
                mReceiveCallback("{\"type\":\"status\",\"message\":\"processing audio\"}");
                break;
            case 4:
                // Error (occasionally)
                if (responseCount % 20 == 0) {
                    mReceiveCallback("{\"type\":\"error\",\"error\":\"simulated error\"}");
                }
                break;
        }
    }

    bool mConnected;
    std::function<void(const std::string&)> mReceiveCallback;
};

// Example listener implementation
class ExampleListener : public IAudioStreamingListener {
public:
    void onReady() override {
        std::cout << "✓ Stream ready to receive audio" << std::endl;
    }

    void onPartialResult(const std::string& text, float stability) override {
        std::cout << "Partial: \"" << text << "\" (stability: " << stability << ")" << std::endl;
    }

    void onFinalResult(const std::string& text, float confidence) override {
        std::cout << "Final: \"" << text << "\" (confidence: " << confidence << ")" << std::endl;
    }

    void onLatency(uint32_t upstreamMs, uint32_t e2eMs) override {
        std::cout << "Latency - Upstream: " << upstreamMs << "ms, E2E: " << e2eMs << "ms" << std::endl;
    }

    void onStatus(const std::string& message) override {
        std::cout << "Status: " << message << std::endl;
    }

    void onError(const std::string& error) override {
        std::cout << "✗ Error: " << error << std::endl;
    }

    void onClosed() override {
        std::cout << "✓ Stream closed" << std::endl;
    }
};

// Generate mock PCM audio data
std::vector<uint8_t> generateMockAudio(size_t samples) {
    std::vector<uint8_t> audio(samples * 2); // 16-bit samples
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int16_t> dis(-32768, 32767);

    for (size_t i = 0; i < samples; ++i) {
        int16_t sample = dis(gen);
        audio[i * 2] = sample & 0xFF;
        audio[i * 2 + 1] = (sample >> 8) & 0xFF;
    }

    return audio;
}

int main() {
    std::cout << "AudioStreaming Module Example" << std::endl;
    std::cout << "=============================" << std::endl;

    // Create configuration
    AudioStreamingConfig config;
    config.sampleRateHz = 16000;
    config.bitsPerSample = 16;
    config.channels = 1;
    config.chunkDurationMs = 20;


    // Create components
    auto listener = std::make_shared<ExampleListener>();
    auto transport = std::make_shared<MockTransportAdapter>();

    // Create AudioStreaming instance
    AudioStreaming streaming(config, listener, transport);

    std::cout << "\nStarting audio streaming session..." << std::endl;
    streaming.start();

    // Wait for ready state
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (!streaming.isActive()) {
        std::cout << "Failed to start streaming session" << std::endl;
        return 1;
    }

    std::cout << "\nStreaming audio data..." << std::endl;

    // Simulate streaming audio for 5 seconds
    const size_t samplesPerChunk = 16000 * 20 / 1000; // 20ms at 16kHz
    uint64_t ptsMs = 0;

    for (int i = 0; i < 250; ++i) { // 250 chunks = 5 seconds
        auto audioData = generateMockAudio(samplesPerChunk);
        streaming.continueWithPcm(audioData.data(), audioData.size(), ptsMs);

        ptsMs += 20; // 20ms chunks
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        // Print progress every second
        if (i % 50 == 0) {
            std::cout << "Progress: " << (i * 20 / 1000) << "s / 5s" << std::endl;
        }
    }

    std::cout << "\nEnding streaming session..." << std::endl;
    streaming.end();

    // Wait for final results
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "\nSession completed!" << std::endl;
    std::cout << "Session ID: " << streaming.getSessionId() << std::endl;
    std::cout << "Final state: " << streaming.getState() << std::endl;

    return 0;
}
