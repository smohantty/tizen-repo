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

void testDecodeBytesBasic() {
    std::cout << "Testing decodeBytes() basic functionality..." << std::endl;

    // Test with a known Base64 string
    std::string base64 = "SGVsbG8gV29ybGQ="; // "Hello World" in Base64
    std::vector<uint8_t> decoded = Base64::decodeBytes(base64);

    // Expected bytes for "Hello World"
    std::vector<uint8_t> expected = {72, 101, 108, 108, 111, 32, 87, 111, 114, 108, 100};

    assert(decoded.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        assert(decoded[i] == expected[i]);
    }

    // Convert to string to verify
    std::string text(decoded.begin(), decoded.end());
    std::cout << "✓ Decoded text: \"" << text << "\"" << std::endl;

    std::cout << "✓ decodeBytes() basic test passed" << std::endl << std::endl;
}

void testDecodeBytesEmpty() {
    std::cout << "Testing decodeBytes() with empty data..." << std::endl;

    std::vector<uint8_t> decoded = Base64::decodeBytes("");
    assert(decoded.empty());
    std::cout << "✓ Empty string decodes to empty byte vector" << std::endl;

    std::cout << "✓ decodeBytes() empty test passed" << std::endl << std::endl;
}

void testDecodeBytesOddLength() {
    std::cout << "Testing decodeBytes() with odd number of bytes..." << std::endl;

    // Encode 3 bytes (odd number, would fail with short version)
    std::vector<short> shortData = {12345}; // 2 bytes
    std::string encoded = Base64::encode(shortData);

    // Manually create a base64 string with 3 bytes
    // "ABC" = 0x41 0x42 0x43 in ASCII -> "QUJD" in Base64
    std::string oddBase64 = "QUJD";
    std::vector<uint8_t> decoded = Base64::decodeBytes(oddBase64);

    assert(decoded.size() == 3);
    assert(decoded[0] == 0x41); // 'A'
    assert(decoded[1] == 0x42); // 'B'
    assert(decoded[2] == 0x43); // 'C'

    std::cout << "✓ Successfully decoded 3 bytes (odd number)" << std::endl;
    std::cout << "✓ Bytes: 0x" << std::hex << (int)decoded[0] << " 0x" << (int)decoded[1] << " 0x" << (int)decoded[2] << std::dec << std::endl;

    // This would fail with decode() for shorts
    try {
        std::vector<short> shortDecoded = Base64::decode(oddBase64);
        std::cout << "✗ Short decode should have failed for odd byte count!" << std::endl;
        assert(false);
    } catch (const std::runtime_error& e) {
        std::cout << "✓ Short decode correctly throws exception for odd bytes: " << e.what() << std::endl;
    }

    std::cout << "✓ decodeBytes() odd length test passed" << std::endl << std::endl;
}

void testDecodeBytesRoundTrip() {
    std::cout << "Testing decodeBytes() round-trip with short data..." << std::endl;

    // Encode shorts to base64
    std::vector<short> originalShorts = {1000, -2000, 3000, -4000};
    std::string encoded = Base64::encode(originalShorts);

    // Decode to bytes
    std::vector<uint8_t> bytes = Base64::decodeBytes(encoded);

    // Should have 8 bytes (4 shorts * 2 bytes each)
    assert(bytes.size() == 8);
    std::cout << "✓ Decoded " << bytes.size() << " bytes from 4 shorts" << std::endl;

    // Decode to shorts
    std::vector<short> decodedShorts = Base64::decode(encoded);
    assert(decodedShorts.size() == originalShorts.size());

    // Verify bytes match the short representation
    const uint8_t* shortBytes = reinterpret_cast<const uint8_t*>(decodedShorts.data());
    for (size_t i = 0; i < bytes.size(); ++i) {
        assert(bytes[i] == shortBytes[i]);
    }

    std::cout << "✓ Byte representation matches short data" << std::endl;
    std::cout << "✓ decodeBytes() round-trip test passed" << std::endl << std::endl;
}

void testDecodeBytesVsShorts() {
    std::cout << "Testing decodeBytes() vs decode() consistency..." << std::endl;

    std::vector<short> shortData = {100, 200, 300, 400, 500};
    std::string encoded = Base64::encode(shortData);

    std::vector<uint8_t> bytes = Base64::decodeBytes(encoded);
    std::vector<short> shorts = Base64::decode(encoded);

    // Bytes should be exactly 2x the number of shorts
    assert(bytes.size() == shorts.size() * 2);
    std::cout << "✓ Byte count (" << bytes.size() << ") = 2 × short count (" << shorts.size() << ")" << std::endl;

    // Verify byte-level representation matches
    const uint8_t* shortBytes = reinterpret_cast<const uint8_t*>(shorts.data());
    bool bytesMatch = true;
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (bytes[i] != shortBytes[i]) {
            bytesMatch = false;
            std::cout << "✗ Byte mismatch at index " << i << std::endl;
            break;
        }
    }

    assert(bytesMatch);
    std::cout << "✓ All bytes match between decodeBytes() and decode()" << std::endl;
    std::cout << "✓ Consistency test passed" << std::endl << std::endl;
}

int main() {
    std::cout << "Starting Base64 tests..." << std::endl << std::endl;

    try {
        // Original short-based tests
        testEmptyData();
        testSingleShort();
        testMultipleShorts();
        testAudioPCMData();
        testRawArrayInterface();
        testEdgeCases();
        testRandomData();

        std::cout << "═══════════════════════════════════════════════" << std::endl;
        std::cout << "Starting decodeBytes() tests..." << std::endl;
        std::cout << "═══════════════════════════════════════════════" << std::endl << std::endl;

        // New byte-based tests
        testDecodeBytesBasic();
        testDecodeBytesEmpty();
        testDecodeBytesOddLength();
        testDecodeBytesRoundTrip();
        testDecodeBytesVsShorts();

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
