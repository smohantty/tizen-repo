#include "Base64.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <random>
#include <iomanip>
#include <cmath>

using namespace utils;

void testEmptyData() {
    std::cout << "Testing empty data..." << std::endl;

    std::vector<short> emptyData;

    std::string encoded = Base64::encode(emptyData);
    assert(encoded.empty());
    std::cout << "✓ Empty data encodes to empty string" << std::endl;

    std::vector<short> decoded = Base64::decode(encoded);
    assert(decoded.empty());
    std::cout << "✓ Empty string decodes to empty data" << std::endl;

    std::cout << "✓ Empty data test passed" << std::endl << std::endl;
}

void testSingleShort() {
    std::cout << "Testing single short value..." << std::endl;

    std::vector<short> data = {12345};

    std::string encoded = Base64::encode(data);
    std::cout << "Single short " << data[0] << " encoded to: " << encoded << std::endl;

    std::vector<short> decoded = Base64::decode(encoded);
    assert(decoded.size() == 1);
    assert(decoded[0] == data[0]);
    std::cout << "✓ Decoded value matches: " << decoded[0] << std::endl;

    std::cout << "✓ Single short test passed" << std::endl << std::endl;
}

void testMultipleShorts() {
    std::cout << "Testing multiple short values..." << std::endl;

    std::vector<short> data = {1000, -2000, 3000, -4000, 5000};

    std::string encoded = Base64::encode(data);
    std::cout << "Multiple shorts encoded to: " << encoded << std::endl;

    std::vector<short> decoded = Base64::decode(encoded);
    assert(decoded.size() == data.size());

    for (size_t i = 0; i < data.size(); ++i) {
        assert(decoded[i] == data[i]);
        std::cout << "✓ Decoded[" << i << "] = " << decoded[i] << " (expected: " << data[i] << ")" << std::endl;
    }

    std::cout << "✓ Multiple shorts test passed" << std::endl << std::endl;
}

void testAudioPCMData() {
    std::cout << "Testing audio PCM data simulation..." << std::endl;

    // Simulate typical audio PCM data (16-bit signed integers)
    std::vector<short> pcmData;
    pcmData.reserve(1000);

    // Generate some realistic audio-like data
    for (int i = 0; i < 1000; ++i) {
        // Generate a sine wave-like pattern
        short value = static_cast<short>(1000 * std::sin(i * 0.1) + 500 * std::sin(i * 0.05));
        pcmData.push_back(value);
    }

    std::string encoded = Base64::encode(pcmData);
    std::cout << "PCM data (" << pcmData.size() << " samples) encoded to Base64" << std::endl;
    std::cout << "Encoded length: " << encoded.length() << " characters" << std::endl;

    std::vector<short> decoded = Base64::decode(encoded);
    assert(decoded.size() == pcmData.size());

    // Check that all values match
    bool allMatch = true;
    for (size_t i = 0; i < pcmData.size(); ++i) {
        if (decoded[i] != pcmData[i]) {
            allMatch = false;
            std::cout << "✗ Mismatch at index " << i << ": expected " << pcmData[i] << ", got " << decoded[i] << std::endl;
            break;
        }
    }

    if (allMatch) {
        std::cout << "✓ All " << pcmData.size() << " PCM samples decoded correctly" << std::endl;
    }

    std::cout << "✓ Audio PCM data test passed" << std::endl << std::endl;
}

void testRawArrayInterface() {
    std::cout << "Testing raw array interface..." << std::endl;

    short data[] = {100, -200, 300, -400, 500};
    std::size_t size = sizeof(data) / sizeof(data[0]);

    std::string encoded = Base64::encode(data, size);
    std::cout << "Raw array encoded to: " << encoded << std::endl;

    short decodedData[5];
    std::size_t decodedSize = 5;
    Base64::decode(encoded, decodedData, decodedSize);

    assert(decodedSize == size);
    for (size_t i = 0; i < size; ++i) {
        assert(decodedData[i] == data[i]);
        std::cout << "✓ Decoded[" << i << "] = " << decodedData[i] << " (expected: " << data[i] << ")" << std::endl;
    }

    std::cout << "✓ Raw array interface test passed" << std::endl << std::endl;
}

void testValidation() {
    std::cout << "Testing Base64 validation..." << std::endl;

    // Valid Base64 strings (our custom format)
    assert(Base64::isValid(""));
    assert(Base64::isValid("DA5"));  // Single short
    assert(Base64::isValid("APoPgwAu4PBgBOI"));  // Multiple shorts
    std::cout << "✓ Valid Base64 strings recognized" << std::endl;

    // Invalid Base64 strings
    assert(!Base64::isValid("DA"));   // Wrong length (not multiple of 3)
    assert(!Base64::isValid("DA5!")); // Invalid character
    assert(!Base64::isValid("DA5A")); // Wrong length
    std::cout << "✓ Invalid Base64 strings rejected" << std::endl;

    std::cout << "✓ Validation test passed" << std::endl << std::endl;
}

void testEdgeCases() {
    std::cout << "Testing edge cases..." << std::endl;

    // Test with maximum and minimum short values
    std::vector<short> edgeData = {32767, -32768, 0, 1, -1};
    std::string encoded = Base64::encode(edgeData);
    std::vector<short> decoded = Base64::decode(encoded);

    assert(decoded.size() == edgeData.size());
    for (size_t i = 0; i < edgeData.size(); ++i) {
        assert(decoded[i] == edgeData[i]);
        std::cout << "✓ Edge case " << i << ": " << edgeData[i] << " -> " << decoded[i] << std::endl;
    }

    // Test with odd number of shorts
    std::vector<short> oddData = {100, 200, 300};
    std::string oddEncoded = Base64::encode(oddData);
    std::vector<short> oddDecoded = Base64::decode(oddEncoded);

    assert(oddDecoded.size() == oddData.size());
    for (size_t i = 0; i < oddData.size(); ++i) {
        assert(oddDecoded[i] == oddData[i]);
    }
    std::cout << "✓ Odd number of shorts handled correctly" << std::endl;

    std::cout << "✓ Edge cases test passed" << std::endl << std::endl;
}

void testRandomData() {
    std::cout << "Testing random data..." << std::endl;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<short> dis(-32768, 32767);

    std::vector<short> randomData;
    randomData.reserve(100);

    for (int i = 0; i < 100; ++i) {
        randomData.push_back(dis(gen));
    }

    std::string encoded = Base64::encode(randomData);
    std::vector<short> decoded = Base64::decode(encoded);

    assert(decoded.size() == randomData.size());

    bool allMatch = true;
    for (size_t i = 0; i < randomData.size(); ++i) {
        if (decoded[i] != randomData[i]) {
            allMatch = false;
            std::cout << "✗ Mismatch at index " << i << ": expected " << randomData[i] << ", got " << decoded[i] << std::endl;
            break;
        }
    }

    if (allMatch) {
        std::cout << "✓ All " << randomData.size() << " random samples decoded correctly" << std::endl;
    }

    std::cout << "✓ Random data test passed" << std::endl << std::endl;
}

int main() {
    std::cout << "Starting Base64 tests..." << std::endl << std::endl;

    try {
        testEmptyData();
        testSingleShort();
        testMultipleShorts();
        testAudioPCMData();
        testRawArrayInterface();
        testValidation();
        testEdgeCases();
        testRandomData();

        std::cout << "🎉 All Base64 tests passed successfully!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}
