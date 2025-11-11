#include "WaveHeader.h"
#include <iostream>
#include <vector>
#include <cstring>

using namespace utils;

int main() {
    std::cout << "=== WAV Header Test ===\n\n";

    // Test 1: Create a new WAV header
    std::cout << "Test 1: Creating a new WAV header (16kHz, mono, 16-bit)\n";
    WaveHeader header(1, 16000, 16, 16000); // 1 second of audio
    std::cout << header.getDescription() << "\n";
    std::cout << "Valid: " << (header.isValid() ? "Yes" : "No") << "\n\n";

    // Test 2: Write to buffer and read back
    std::cout << "Test 2: Write to buffer and read back\n";
    char buffer[44];
    header.writeToBuffer(buffer);

    WaveHeader header2;
    if (header2.readFromBuffer(buffer)) {
        std::cout << "Successfully read from buffer\n";
        std::cout << "Channels: " << header2.mNumChannels << "\n";
        std::cout << "Sample Rate: " << header2.mSampleRate << "\n";
        std::cout << "Bits Per Sample: " << header2.mBitsPerSample << "\n";
        std::cout << "Valid: " << (header2.isValid() ? "Yes" : "No") << "\n\n";
    } else {
        std::cout << "Failed to read from buffer\n\n";
    }

    // Test 3: Create stereo 48kHz 24-bit header
    std::cout << "Test 3: Creating stereo 48kHz 24-bit header\n";
    WaveHeader header3(2, 48000, 24, 48000); // 1 second of stereo audio
    std::cout << header3.getDescription() << "\n";
    std::cout << "Valid: " << (header3.isValid() ? "Yes" : "No") << "\n\n";

    // Test 4: Test file I/O
    std::cout << "Test 4: Writing to file and reading back\n";
    const std::string testFile = "test_wave_header.wav";

    if (header3.writeToFile(testFile)) {
        std::cout << "Successfully wrote header to " << testFile << "\n";

        WaveHeader header4;
        if (header4.readFromFile(testFile)) {
            std::cout << "Successfully read header from file\n";
            std::cout << "Channels: " << header4.mNumChannels << "\n";
            std::cout << "Sample Rate: " << header4.mSampleRate << "\n";
            std::cout << "Bits Per Sample: " << header4.mBitsPerSample << "\n";
            std::cout << "Duration: " << header4.getDuration() << " seconds\n";
        } else {
            std::cout << "Failed to read from file\n";
        }
    } else {
        std::cout << "Failed to write to file\n";
    }

    std::cout << "\n=== All tests completed ===\n";

    return 0;
}

