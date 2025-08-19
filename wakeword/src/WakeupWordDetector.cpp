#include "WakeupWordDetector.h"
#include <algorithm>

namespace wakeword {

namespace {
constexpr size_t kMinChunkSize = 1600; // Example: 100ms at 16kHz (1 frame)
constexpr size_t kFirstProcessorStartFrames = 3; // Start after 3 frames
constexpr size_t kSecondProcessorFrames = 130; // Process last 130 frames
}

class WakeupWordDetector::Impl {
public:
    bool mDetected = false;
    std::vector<short> mBuffer;
    size_t mFramesProcessed = 0;

    // First processor: sliding window processing after 3 frames
    bool processFirstStage(const std::vector<short>& frame) {
        // Simple threshold-based detection for first stage
        for (short sample : frame) {
            if (sample > 29000) {
                return true;
            }
        }
        return false;
    }

    // Second processor: processes last 130 frames
    bool processSecondStage(const std::vector<short>& frames) {
        // More sophisticated processing for 130 frames
        // Example: energy-based detection
        long long totalEnergy = 0;
        for (short sample : frames) {
            totalEnergy += static_cast<long long>(sample) * sample;
        }
        double avgEnergy = static_cast<double>(totalEnergy) / frames.size();
        return avgEnergy > 500000000.0; // Example threshold
    }

    bool processAudioFrames() {
        bool firstStageDetected = false;
        bool secondStageDetected = false;

        // First processor: sliding window after 3 frames
        if (mFramesProcessed >= kFirstProcessorStartFrames) {
            size_t currentFrameStart = (mFramesProcessed - 1) * kMinChunkSize;
            if (currentFrameStart + kMinChunkSize <= mBuffer.size()) {
                std::vector<short> currentFrame(
                    mBuffer.begin() + currentFrameStart,
                    mBuffer.begin() + currentFrameStart + kMinChunkSize
                );
                firstStageDetected = processFirstStage(currentFrame);
            }
        }

        // Second processor: process last 130 frames when available
        if (mFramesProcessed >= kSecondProcessorFrames) {
            size_t secondStageSize = kSecondProcessorFrames * kMinChunkSize;
            if (mBuffer.size() >= secondStageSize) {
                std::vector<short> last130Frames(
                    mBuffer.end() - secondStageSize,
                    mBuffer.end()
                );
                secondStageDetected = processSecondStage(last130Frames);
            }
        }

        mDetected = firstStageDetected || secondStageDetected;
        return mDetected;
    }

    bool isWakeupWordDetected() const {
        return mDetected;
    }
};

WakeupWordDetector::WakeupWordDetector()
    : mImpl(std::make_unique<Impl>()) {}

WakeupWordDetector::~WakeupWordDetector() = default;

bool WakeupWordDetector::processAudioBuffer(const std::vector<short>& buffer) {
    mImpl->mBuffer.insert(mImpl->mBuffer.end(), buffer.begin(), buffer.end());
    bool detected = false;

    // Process complete frames
    while (mImpl->mBuffer.size() >= kMinChunkSize) {
        mImpl->mFramesProcessed++;
        detected = mImpl->processAudioFrames() || detected;

        // Remove the oldest frame for sliding window, but keep enough for second processor
        size_t maxBufferSize = kSecondProcessorFrames * kMinChunkSize;
        if (mImpl->mBuffer.size() > maxBufferSize) {
            mImpl->mBuffer.erase(mImpl->mBuffer.begin(),
                               mImpl->mBuffer.begin() + kMinChunkSize);
        } else {
            // If we haven't reached max buffer size, just advance frame count
            break;
        }
    }

    return detected;
}

bool WakeupWordDetector::isWakeupWordDetected() const {
    return mImpl->isWakeupWordDetected();
}

} // namespace wakeword
