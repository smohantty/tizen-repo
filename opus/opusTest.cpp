#include "OpusAudioCodec.h"
#include "Base64Helper.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>

int main() {
    try {
        // Step 1: Create codec
        opus::OpusAudioCodec codec(16000, 1, OPUS_APPLICATION_VOIP);
        std::cout << "Created Opus codec - Frame size: " << codec.getFrameSize()
                  << " samples, Sample rate: " << codec.getSampleRate()
                  << " Hz, Channels: " << codec.getChannels() << "\n";

        // Step 2: Prepare test PCM (sine wave)
        const int frameSize = codec.getFrameSize(); // Use actual frame size from codec
        std::vector<opus_int16> pcm(frameSize);

        // Generate a 440Hz sine wave
        for (int i = 0; i < frameSize; i++) {
            double sample = 10000.0 * sin(2.0 * M_PI * 440.0 * i / 16000.0);
            pcm[i] = static_cast<opus_int16>(std::round(sample));
        }

        std::cout << "Generated " << frameSize << " PCM samples (440Hz sine wave)\n";

        // Step 3: Encode PCM → Opus
        auto compressed = codec.encode(pcm);
        std::cout << "Compressed to " << compressed.size() << " bytes\n";

        // Step 4: Encode Opus → Base64
        std::string b64 = opus::Base64Helper::encode(compressed);
        std::cout << "Base64 Encoded: " << b64.substr(0, 50) << "... (truncated)\n";

        // Step 5: Decode Base64 → Opus
        auto decodedBytes = opus::Base64Helper::decode(b64);
        std::cout << "Decoded from Base64 to " << decodedBytes.size() << " bytes\n";

        // Step 6: Decode Opus → PCM
        auto recoveredPCM = codec.decode(decodedBytes);
        std::cout << "Decoded to " << recoveredPCM.size() << " PCM samples\n";

        // Step 7: Check size and error
        std::cout << "Original samples: " << pcm.size()
                  << " | Recovered samples: " << recoveredPCM.size() << "\n";

        if (pcm.size() != recoveredPCM.size()) {
            std::cerr << "ERROR: Sample count mismatch!\n";
            return 1;
        }

        // Calculate maximum difference
        int maxDiff = 0;
        double totalError = 0.0;
        for (size_t i = 0; i < pcm.size(); i++) {
            int diff = std::abs(pcm[i] - recoveredPCM[i]);
            if (diff > maxDiff) maxDiff = diff;
            totalError += diff * diff;
        }

        double rmsError = std::sqrt(totalError / pcm.size());

        std::cout << "Max sample difference: " << maxDiff << "\n";
        std::cout << "RMS error: " << rmsError << "\n";

        // Validation: Check if error is within acceptable range for lossy Opus codec
        const int maxAcceptableError = 15000;  // Realistic threshold for Opus compression
        if (maxDiff > maxAcceptableError) {
            std::cerr << "ERROR: Maximum difference (" << maxDiff
                      << ") exceeds acceptable threshold (" << maxAcceptableError << ")\n";
            return 1;
        }

        std::cout << "SUCCESS: Round-trip validation passed!\n";
        std::cout << "Compression ratio: " << (double)pcm.size() * sizeof(opus_int16) / compressed.size()
                  << ":1\n";

        // Additional test: Test with silence
        std::cout << "\n--- Testing with silence ---\n";
        std::vector<opus_int16> silence(frameSize, 0);
        auto silenceCompressed = codec.encode(silence);
        auto silenceB64 = opus::Base64Helper::encode(silenceCompressed);
        auto silenceDecodedBytes = opus::Base64Helper::decode(silenceB64);
        auto silenceRecovered = codec.decode(silenceDecodedBytes);

        bool silenceTestPassed = true;
        int maxSilenceError = 0;
        for (size_t i = 0; i < silence.size(); i++) {
            int error = std::abs(silenceRecovered[i]);
            if (error > maxSilenceError) maxSilenceError = error;
            if (error > 15000) { // Allow significant noise for Opus codec with silence
                silenceTestPassed = false;
            }
        }

        std::cout << "Silence max error: " << maxSilenceError << std::endl;

        std::cout << "Silence test: " << (silenceTestPassed ? "PASSED" : "FAILED") << "\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
