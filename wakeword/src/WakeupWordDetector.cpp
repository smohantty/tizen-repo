#include "WakeupWordDetector.h"
#include <algorithm>
#include <stdexcept>

namespace wakeword {

namespace {
// Configuration structure - implementation detail
struct Config {
    std::string modelFilePath;
    size_t frameSize = 1600;              // Samples per frame (default: 100ms at 16kHz)
    size_t firstProcessorStartFrames = 3; // Start first processor after N frames
    size_t secondProcessorFrames = 130;   // Second processor uses last N frames
    int16_t firstStageThreshold = 29000;  // Threshold for first stage detection
    double secondStageEnergyThreshold = 500000000.0; // Energy threshold for second stage
    int sampleRate = 16000;               // Sample rate in Hz
    int channels = 1;                     // Number of audio channels

    explicit Config(const std::string& modelPath) : modelFilePath(modelPath) {
        if (modelPath.empty()) {
            throw std::invalid_argument("Model file path cannot be empty");
        }
    }
};

constexpr size_t kMinChunkSize = 1600; // Will be replaced by config.frameSize
constexpr size_t kFirstProcessorStartFrames = 3; // Will be replaced by config values
constexpr size_t kSecondProcessorFrames = 130; // Will be replaced by config values
}

class WakeupWordDetector::Impl {
public:
    Config mConfig;
    bool mDetected = false;
    std::vector<short> mBuffer;
    size_t mFramesProcessed = 0;

    explicit Impl(const std::string& modelFilePath) : mConfig(modelFilePath) {}

    // First processor: sliding window processing after configured frames
    bool processFirstStage(const std::vector<short>& frame) {
        // Simple threshold-based detection for first stage
        for (short sample : frame) {
            if (sample > mConfig.firstStageThreshold) {
                return true;
            }
        }
        return false;
    }

    // Second processor: processes last N frames
    bool processSecondStage(const std::vector<short>& frames) {
        // More sophisticated processing for configured number of frames
        // Example: energy-based detection
        long long totalEnergy = 0;
        for (short sample : frames) {
            totalEnergy += static_cast<long long>(sample) * sample;
        }
        double avgEnergy = static_cast<double>(totalEnergy) / frames.size();
        return avgEnergy > mConfig.secondStageEnergyThreshold;
    }

    bool processAudioFrames() {
        bool firstStageDetected = false;
        bool secondStageDetected = false;

        // First processor: sliding window after configured frames
        if (mFramesProcessed >= mConfig.firstProcessorStartFrames) {
            size_t currentFrameStart = (mFramesProcessed - 1) * mConfig.frameSize;
            if (currentFrameStart + mConfig.frameSize <= mBuffer.size()) {
                std::vector<short> currentFrame(
                    mBuffer.begin() + currentFrameStart,
                    mBuffer.begin() + currentFrameStart + mConfig.frameSize
                );
                firstStageDetected = processFirstStage(currentFrame);
            }
        }

        // Second processor: process last N frames when available
        if (mFramesProcessed >= mConfig.secondProcessorFrames) {
            size_t secondStageSize = mConfig.secondProcessorFrames * mConfig.frameSize;
            if (mBuffer.size() >= secondStageSize) {
                std::vector<short> lastFrames(
                    mBuffer.end() - secondStageSize,
                    mBuffer.end()
                );
                secondStageDetected = processSecondStage(lastFrames);
            }
        }

        mDetected = firstStageDetected || secondStageDetected;
        return mDetected;
    }

    bool isWakeupWordDetected() const {
        return mDetected;
    }

    // Main processing method moved from WakeupWordDetector
    bool processAudioBuffer(const std::vector<short>& buffer) {
        mBuffer.insert(mBuffer.end(), buffer.begin(), buffer.end());
        bool detected = false;

        // Process complete frames
        while (mBuffer.size() >= mConfig.frameSize) {
            mFramesProcessed++;
            detected = processAudioFrames() || detected;

            // Remove the oldest frame for sliding window, but keep enough for second processor
            size_t maxBufferSize = mConfig.secondProcessorFrames * mConfig.frameSize;
            if (mBuffer.size() > maxBufferSize) {
                mBuffer.erase(mBuffer.begin(),
                             mBuffer.begin() + mConfig.frameSize);
            } else {
                // If we haven't reached max buffer size, just advance frame count
                break;
            }
        }

        return detected;
    }
};

WakeupWordDetector::WakeupWordDetector(const std::string& modelFilePath)
    : mImpl(std::make_unique<Impl>(modelFilePath)) {}

WakeupWordDetector::~WakeupWordDetector() = default;

bool WakeupWordDetector::processAudioBuffer(const std::vector<short>& buffer) {
    return mImpl->processAudioBuffer(buffer);
}

bool WakeupWordDetector::isWakeupWordDetected() const {
    return mImpl->isWakeupWordDetected();
}

} // namespace wakeword
