#include "AudioStreamer.h"
#include <iostream>

int main() {
    AudioStreamer streamer(10); // 10 ms chunks, 16 kHz mono

    std::cout << "Starting audio streamer...\n";
    streamer.start();

    std::vector<short> chunk;
    while (streamer.popChunk(chunk)) {
        std::cout << "Received chunk with " << chunk.size() << " samples, first sample: " << chunk[0] << "\n";
        // Process chunk here...
    }

    // Check if we exited due to an error or normal stop
    if (!streamer.isRunning()) {
        std::cout << "Audio streamer stopped (possibly due to command failure)\n";
    }

    streamer.stop();
    std::cout << "Audio streamer terminated gracefully\n";
    return 0;
}
