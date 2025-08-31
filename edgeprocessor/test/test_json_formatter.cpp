#include "JsonFormatter.h"
#include "AudioStreamingConfig.h"
#include "Message.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace edgeprocessor;

void testStartMessageFormatting() {
    std::cout << "Testing start message formatting..." << std::endl;

    JsonFormatter formatter;
    AudioStreamingConfig config;
    config.sampleRateHz = 16000;
    config.bitsPerSample = 16;
    config.channels = 1;
    config.sessionId = "test-session-123";

    std::string json = formatter.formatStart(config);

    // Check that all required fields are present
    assert(json.find("\"type\":\"start\"") != std::string::npos);
    assert(json.find("\"session_id\":\"test-session-123\"") != std::string::npos);
    assert(json.find("\"sample_rate_hz\":16000") != std::string::npos);
    assert(json.find("\"bits_per_sample\":16") != std::string::npos);
    assert(json.find("\"channels\":1") != std::string::npos);
    assert(json.find("\"partial_results\":true") != std::string::npos);
    assert(json.find("\"compression\":\"pcm16\"") != std::string::npos);

    std::cout << "✓ Start message formatting test passed" << std::endl;
}

void testAudioMessageFormatting() {
    std::cout << "Testing audio message formatting..." << std::endl;

    JsonFormatter formatter;
    std::vector<uint8_t> audioData = {1, 2, 3, 4, 5};

    std::string json = formatter.formatAudio(audioData, 12345, 42, false);

    // Check that all required fields are present
    assert(json.find("\"type\":\"audio\"") != std::string::npos);
    assert(json.find("\"seq\":42") != std::string::npos);
    assert(json.find("\"pts_ms\":12345") != std::string::npos);
    assert(json.find("\"last\":false") != std::string::npos);
    assert(json.find("\"payload\":") != std::string::npos);

    // Test with last=true
    json = formatter.formatAudio(audioData, 12345, 42, true);
    assert(json.find("\"last\":true") != std::string::npos);

    std::cout << "✓ Audio message formatting test passed" << std::endl;
}

void testEndMessageFormatting() {
    std::cout << "Testing end message formatting..." << std::endl;

    JsonFormatter formatter;
    std::string json = formatter.formatEnd(99);

    // Check that all required fields are present
    assert(json.find("\"type\":\"end\"") != std::string::npos);
    assert(json.find("\"seq\":99") != std::string::npos);
    assert(json.find("\"last\":true") != std::string::npos);

    std::cout << "✓ End message formatting test passed" << std::endl;
}

void testPartialResultParsing() {
    std::cout << "Testing partial result parsing..." << std::endl;

    JsonFormatter formatter;
    std::string json = "{\"type\":\"partial\",\"text\":\"hello\",\"stability\":0.85}";

    std::cout << "Input JSON: " << json << std::endl;
    std::cout << "JSON length: " << json.length() << std::endl;

    EvPartial result = formatter.parsePartial(json);

    std::cout << "Parsed text: '" << result.text << "'" << std::endl;
    std::cout << "Parsed stability: " << result.stability << std::endl;

    assert(result.text == "hello");
    assert(result.stability == 0.85f);

    std::cout << "✓ Partial result parsing test passed" << std::endl;
}

void testFinalResultParsing() {
    std::cout << "Testing final result parsing..." << std::endl;

    JsonFormatter formatter;
    std::string json = "{\"type\":\"final\",\"text\":\"hello\",\"confidence\":0.94}";

    EvFinal result = formatter.parseFinal(json);

    assert(result.text == "hello");
    assert(result.confidence == 0.94f);

    std::cout << "✓ Final result parsing test passed" << std::endl;
}

void testLatencyParsing() {
    std::cout << "Testing latency parsing..." << std::endl;

    JsonFormatter formatter;
    std::string json = "{\"type\":\"latency\",\"upstream_ms\":42,\"e2e_ms\":120}";

    EvLatency result = formatter.parseLatency(json);

    assert(result.upstreamMs == 42);
    assert(result.e2eMs == 120);

    std::cout << "✓ Latency parsing test passed" << std::endl;
}

void testStatusParsing() {
    std::cout << "Testing status parsing..." << std::endl;

    JsonFormatter formatter;
    std::string json = "{\"type\":\"status\",\"message\":\"processing audio\"}";

    EvStatus result = formatter.parseStatus(json);

    assert(result.message == "processing audio");

    std::cout << "✓ Status parsing test passed" << std::endl;
}

void testErrorParsing() {
    std::cout << "Testing error parsing..." << std::endl;

    JsonFormatter formatter;
    std::string json = "{\"type\":\"error\",\"error\":\"connection failed\"}";

    EvError result = formatter.parseError(json);

    assert(result.what == "connection failed");

    std::cout << "✓ Error parsing test passed" << std::endl;
}

void testBase64Encoding() {
    std::cout << "Testing Base64 encoding..." << std::endl;

    // Test empty data
    std::vector<uint8_t> empty;
    std::string encoded = JsonFormatter::encodeBase64(empty);
    assert(encoded.empty());

    // Test simple data
    std::vector<uint8_t> data = {0x00, 0x01, 0x02};
    encoded = JsonFormatter::encodeBase64(data);
    assert(encoded == "AAEC");

    // Test data that needs padding
    data = {0x00, 0x01};
    encoded = JsonFormatter::encodeBase64(data);
    assert(encoded == "AAE=");

    // Test single byte
    data = {0x00};
    encoded = JsonFormatter::encodeBase64(data);
    assert(encoded == "AA==");

    std::cout << "✓ Base64 encoding test passed" << std::endl;
}

void testBase64Decoding() {
    std::cout << "Testing Base64 decoding..." << std::endl;

    // Test empty string
    std::vector<uint8_t> decoded = JsonFormatter::decodeBase64("");
    assert(decoded.empty());

    // Test simple data
    decoded = JsonFormatter::decodeBase64("AAEC");
    assert(decoded.size() == 3);
    assert(decoded[0] == 0x00);
    assert(decoded[1] == 0x01);
    assert(decoded[2] == 0x02);

    // Test data with padding
    decoded = JsonFormatter::decodeBase64("AAE=");
    assert(decoded.size() == 2);
    assert(decoded[0] == 0x00);
    assert(decoded[1] == 0x01);

    // Test single byte with padding
    decoded = JsonFormatter::decodeBase64("AA==");
    assert(decoded.size() == 1);
    assert(decoded[0] == 0x00);

    std::cout << "✓ Base64 decoding test passed" << std::endl;
}

void testUuidGeneration() {
    std::cout << "Testing UUID generation..." << std::endl;

    std::string uuid1 = JsonFormatter::generateUuid();
    std::string uuid2 = JsonFormatter::generateUuid();

    // Check format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    assert(uuid1.length() == 36);
    assert(uuid1[8] == '-');
    assert(uuid1[13] == '-');
    assert(uuid1[18] == '-');
    assert(uuid1[23] == '-');

    // UUIDs should be different
    assert(uuid1 != uuid2);

    // Check that all characters are hex digits or hyphens
    for (char c : uuid1) {
        assert((c >= '0' && c <= '9') ||
               (c >= 'a' && c <= 'f') ||
               (c >= 'A' && c <= 'F') ||
               c == '-');
    }

    std::cout << "✓ UUID generation test passed" << std::endl;
}

int main() {
    std::cout << "JsonFormatter Unit Tests" << std::endl;
    std::cout << "=======================" << std::endl;

    try {
        testStartMessageFormatting();
        testAudioMessageFormatting();
        testEndMessageFormatting();
        testPartialResultParsing();
        testFinalResultParsing();
        testLatencyParsing();
        testStatusParsing();
        testErrorParsing();
        testBase64Encoding();
        testBase64Decoding();
        testUuidGeneration();

        std::cout << "\n✓ All JsonFormatter tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
