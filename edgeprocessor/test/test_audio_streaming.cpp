#include "AudioStreaming.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using namespace edgeprocessor;

// Mock transport adapter for testing
class MockTransportAdapter : public ITransportAdapter {
public:
    MockTransportAdapter() : mConnected(true), mReceiveCallback(nullptr) {}

    bool send(const std::string& jsonMessage) override {
        mLastSentMessage = jsonMessage;
        return mConnected;
    }

    void setReceiveCallback(std::function<void(const std::string&)> callback) override {
        mReceiveCallback = callback;
    }

    bool isConnected() const override {
        return mConnected;
    }

    void setConnected(bool connected) {
        mConnected = connected;
    }

    void simulateReceive(const std::string& message) {
        if (mReceiveCallback) {
            mReceiveCallback(message);
        }
    }

    std::string getLastSentMessage() const {
        return mLastSentMessage;
    }

private:
    bool mConnected;
    std::function<void(const std::string&)> mReceiveCallback;
    std::string mLastSentMessage;
};

// Test listener
class TestListener : public IAudioStreamingListener {
public:
    void onReady() override { mReadyCalled = true; }
    void onPartialResult(const std::string& text, float stability) override {
        mPartialResults.push_back({text, stability});
    }
    void onFinalResult(const std::string& text, float confidence) override {
        mFinalResults.push_back({text, confidence});
    }
    void onLatency(uint32_t upstreamMs, uint32_t e2eMs) override {
        mLatencyReports.push_back({upstreamMs, e2eMs});
    }
    void onStatus(const std::string& message) override {
        mStatusMessages.push_back(message);
    }
    void onError(const std::string& error) override {
        mErrors.push_back(error);
    }
    void onClosed() override { mClosedCalled = true; }

    bool mReadyCalled = false;
    bool mClosedCalled = false;
    std::vector<std::pair<std::string, float>> mPartialResults;
    std::vector<std::pair<std::string, float>> mFinalResults;
    std::vector<std::pair<uint32_t, uint32_t>> mLatencyReports;
    std::vector<std::string> mStatusMessages;
    std::vector<std::string> mErrors;
};

void testBasicStateTransitions() {
    std::cout << "Testing basic state transitions..." << std::endl;

    AudioStreamingConfig config;
    auto listener = std::make_shared<TestListener>();
    auto transport = std::make_shared<MockTransportAdapter>();

    AudioStreaming streaming(config, listener, transport);

    // Initial state should be Idle
    assert(streaming.getState() == "Idle");
    assert(!streaming.isActive());

    // Start should transition to Ready
    streaming.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    assert(streaming.getState() == "Ready");
    assert(streaming.isActive());
    assert(listener->mReadyCalled);

    // End should transition to Ending
    streaming.end();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    assert(streaming.getState() == "Ending");

    // Simulate final result to close
    transport->simulateReceive("{\"type\":\"final\",\"text\":\"test\",\"confidence\":0.9}");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    assert(streaming.getState() == "Closed");
    assert(listener->mClosedCalled);

    std::cout << "✓ Basic state transitions test passed" << std::endl;
}

void testAudioStreaming() {
    std::cout << "Testing audio streaming..." << std::endl;

    AudioStreamingConfig config;
    auto listener = std::make_shared<TestListener>();
    auto transport = std::make_shared<MockTransportAdapter>();

    AudioStreaming streaming(config, listener, transport);

    streaming.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Send audio data
    std::vector<uint8_t> audioData = {1, 2, 3, 4, 5};
    streaming.continueWithPcm(audioData.data(), audioData.size(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Should be in Streaming state
    assert(streaming.getState() == "Streaming");

    // Check that audio message was sent
    std::string lastMessage = transport->getLastSentMessage();
    assert(lastMessage.find("\"type\":\"audio\"") != std::string::npos);
    assert(lastMessage.find("\"seq\":1") != std::string::npos);

    streaming.end();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "✓ Audio streaming test passed" << std::endl;
}

void testErrorHandling() {
    std::cout << "Testing error handling..." << std::endl;

    AudioStreamingConfig config;
    auto listener = std::make_shared<TestListener>();
    auto transport = std::make_shared<MockTransportAdapter>();

    AudioStreaming streaming(config, listener, transport);

    // Disconnect transport
    transport->setConnected(false);

    streaming.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Should be in Closed state due to transport error
    assert(streaming.getState() == "Closed");
    assert(!listener->mErrors.empty());

    std::cout << "✓ Error handling test passed" << std::endl;
}

void testJsonParsing() {
    std::cout << "Testing JSON parsing..." << std::endl;

    AudioStreamingConfig config;
    auto listener = std::make_shared<TestListener>();
    auto transport = std::make_shared<MockTransportAdapter>();

    AudioStreaming streaming(config, listener, transport);

    streaming.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Test partial result
    transport->simulateReceive("{\"type\":\"partial\",\"text\":\"hello\",\"stability\":0.8}");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    assert(listener->mPartialResults.size() == 1);
    assert(listener->mPartialResults[0].first == "hello");
    assert(listener->mPartialResults[0].second == 0.8f);

    // Test final result
    transport->simulateReceive("{\"type\":\"final\",\"text\":\"hello world\",\"confidence\":0.95}");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    assert(listener->mFinalResults.size() == 1);
    assert(listener->mFinalResults[0].first == "hello world");
    assert(listener->mFinalResults[0].second == 0.95f);

    // Test latency
    transport->simulateReceive("{\"type\":\"latency\",\"upstream_ms\":50,\"e2e_ms\":150}");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    assert(listener->mLatencyReports.size() == 1);
    assert(listener->mLatencyReports[0].first == 50);
    assert(listener->mLatencyReports[0].second == 150);

    streaming.end();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "✓ JSON parsing test passed" << std::endl;
}

int main() {
    std::cout << "AudioStreaming Unit Tests" << std::endl;
    std::cout << "========================" << std::endl;

    try {
        testBasicStateTransitions();
        testAudioStreaming();
        testErrorHandling();
        testJsonParsing();

        std::cout << "\n✓ All AudioStreaming tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
