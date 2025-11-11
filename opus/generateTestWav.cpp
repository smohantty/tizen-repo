#include <iostream>
#include <vector>
#include <cmath>
#include <sndfile.h>

int main(int argc, char* argv[]) {
    std::string outputFile = "input.wav";
    int sampleRate = 16000;
    int channels = 1;
    int durationSeconds = 3;

    if (argc > 1) {
        outputFile = argv[1];
    }
    if (argc > 2) {
        durationSeconds = std::stoi(argv[2]);
    }

    std::cout << "Generating test WAV file: " << outputFile << "\n";
    std::cout << "  Sample rate: " << sampleRate << " Hz\n";
    std::cout << "  Channels: " << channels << "\n";
    std::cout << "  Duration: " << durationSeconds << " seconds\n";

    // Calculate total samples
    int totalSamples = sampleRate * durationSeconds * channels;
    std::vector<short> samples(totalSamples);

    // Generate a composite signal with multiple frequencies
    // This creates a more interesting test signal
    for (int i = 0; i < totalSamples; i++) {
        double t = static_cast<double>(i) / sampleRate;
        double sample = 0.0;

        // Mix of frequencies: 440Hz (A4), 554Hz (C#5), 659Hz (E5) - A major chord
        sample += 8000.0 * std::sin(2.0 * M_PI * 440.0 * t);
        sample += 6000.0 * std::sin(2.0 * M_PI * 554.37 * t);
        sample += 4000.0 * std::sin(2.0 * M_PI * 659.25 * t);

        // Add some envelope to make it more natural
        double envelope = 1.0;
        if (t < 0.1) {
            // Attack
            envelope = t / 0.1;
        } else if (t > durationSeconds - 0.2) {
            // Release
            envelope = (durationSeconds - t) / 0.2;
        }

        samples[i] = static_cast<short>(sample * envelope);
    }

    // Write to WAV file using libsndfile
    SF_INFO sfinfo;
    sfinfo.samplerate = sampleRate;
    sfinfo.channels = channels;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* outfile = sf_open(outputFile.c_str(), SFM_WRITE, &sfinfo);
    if (!outfile) {
        std::cerr << "Failed to open output file: " << outputFile << "\n";
        std::cerr << "Error: " << sf_strerror(nullptr) << "\n";
        return 1;
    }

    sf_count_t written = sf_write_short(outfile, samples.data(), samples.size());
    sf_close(outfile);

    if (written != static_cast<sf_count_t>(samples.size())) {
        std::cerr << "Warning: Only wrote " << written << " out of " << samples.size() << " samples\n";
    }

    std::cout << "Successfully generated " << written << " samples\n";
    std::cout << "File saved to: " << outputFile << "\n";

    return 0;
}

