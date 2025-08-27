#include "VoiceActivityDetector.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>

namespace vad {

namespace {
    // Default configuration constants
    constexpr double kDefaultEnergyThreshold = 1000000.0;
    constexpr uint64_t kDefaultMinSpeechDurationMs = 100;
    constexpr uint64_t kDefaultMinSilenceDurationMs = 200;
    constexpr size_t kSmoothingWindowFrames = 3;
}

class VoiceActivityDetector::Impl {
public:
    // Configuration
    int mSampleRate;
    size_t mFrameSize;
    double mEnergyThreshold;
    uint64_t mMinSpeechDurationMs;
    uint64_t mMinSilenceDurationMs;

    // State
    std::vector<short> mBuffer;
    bool mIsSpeechActive;
    uint64_t mLastStateChangeTimestamp;
    uint64_t mCurrentTimestamp;
    uint64_t mSpeechStartTimestamp;
    uint64_t mSilenceStartTimestamp;

    // Smoothing
    std::vector<bool> mRecentDetections;
    size_t mDetectionIndex;

    // Callback
    SpeechEventCallback mCallback;

    explicit Impl(int sampleRate, size_t frameSize)
        : mSampleRate(sampleRate)
        , mFrameSize(frameSize)
        , mEnergyThreshold(kDefaultEnergyThreshold)
        , mMinSpeechDurationMs(kDefaultMinSpeechDurationMs)
        , mMinSilenceDurationMs(kDefaultMinSilenceDurationMs)
        , mIsSpeechActive(false)
        , mLastStateChangeTimestamp(0)
        , mCurrentTimestamp(0)
        , mSpeechStartTimestamp(0)
        , mSilenceStartTimestamp(0)
        , mDetectionIndex(0)
    {
        mRecentDetections.resize(kSmoothingWindowFrames, false);
    }

    bool processAudioBuffer(const std::vector<short>& buffer) {
        mBuffer.insert(mBuffer.end(), buffer.begin(), buffer.end());

        bool stateChanged = false;

        // Process complete frames
        while (mBuffer.size() >= mFrameSize) {
            std::vector<short> frame(mBuffer.begin(), mBuffer.begin() + mFrameSize);

            bool frameHasSpeech = detectVoiceActivity(frame);
            bool smoothedDetection = updateSmoothing(frameHasSpeech);

            stateChanged = updateSpeechState(smoothedDetection) || stateChanged;

            // Remove processed frame
            mBuffer.erase(mBuffer.begin(), mBuffer.begin() + mFrameSize);

            // Update timestamp (frame duration in milliseconds)
            mCurrentTimestamp += (mFrameSize * 1000) / mSampleRate;
        }

        return stateChanged;
    }

private:
    bool detectVoiceActivity(const std::vector<short>& frame) {
        // Calculate RMS energy
        double energy = 0.0;
        for (short sample : frame) {
            energy += static_cast<double>(sample) * sample;
        }
        energy = std::sqrt(energy / frame.size());

        return energy > mEnergyThreshold;
    }

    bool updateSmoothing(bool detection) {
        // Add current detection to smoothing window
        mRecentDetections[mDetectionIndex] = detection;
        mDetectionIndex = (mDetectionIndex + 1) % kSmoothingWindowFrames;

        // Count positive detections in window
        size_t positiveDetections = std::count(mRecentDetections.begin(), mRecentDetections.end(), true);

        // Majority vote for smoothing
        return positiveDetections > (kSmoothingWindowFrames / 2);
    }

    bool updateSpeechState(bool detected) {
        bool stateChanged = false;

        if (!mIsSpeechActive && detected) {
            // Potential speech start
            if (mSpeechStartTimestamp == 0) {
                mSpeechStartTimestamp = mCurrentTimestamp;
            } else if (mCurrentTimestamp - mSpeechStartTimestamp >= mMinSpeechDurationMs) {
                // Speech confirmed after minimum duration
                mIsSpeechActive = true;
                mLastStateChangeTimestamp = mCurrentTimestamp;
                mSilenceStartTimestamp = 0;
                stateChanged = true;

                if (mCallback) {
                    mCallback(true, mCurrentTimestamp);
                }
            }
        } else if (mIsSpeechActive && !detected) {
            // Potential speech end
            if (mSilenceStartTimestamp == 0) {
                mSilenceStartTimestamp = mCurrentTimestamp;
            } else if (mCurrentTimestamp - mSilenceStartTimestamp >= mMinSilenceDurationMs) {
                // Silence confirmed after minimum duration
                mIsSpeechActive = false;
                mLastStateChangeTimestamp = mCurrentTimestamp;
                mSpeechStartTimestamp = 0;
                stateChanged = true;

                if (mCallback) {
                    mCallback(false, mCurrentTimestamp);
                }
            }
        } else if (!detected) {
            // Reset speech start tracking if no detection
            mSpeechStartTimestamp = 0;
        } else if (detected) {
            // Reset silence start tracking if detection continues
            mSilenceStartTimestamp = 0;
        }

        return stateChanged;
    }

public:
    bool isSpeechActive() const {
        return mIsSpeechActive;
    }

    uint64_t getLastStateChangeTimestamp() const {
        return mLastStateChangeTimestamp;
    }

    void setSpeechEventCallback(const SpeechEventCallback& callback) {
        mCallback = callback;
    }

    void reset() {
        mBuffer.clear();
        mIsSpeechActive = false;
        mLastStateChangeTimestamp = 0;
        mCurrentTimestamp = 0;
        mSpeechStartTimestamp = 0;
        mSilenceStartTimestamp = 0;
        mDetectionIndex = 0;
        std::fill(mRecentDetections.begin(), mRecentDetections.end(), false);
    }

    void setEnergyThreshold(double threshold) {
        mEnergyThreshold = threshold;
    }

    void setMinSpeechDuration(uint64_t durationMs) {
        mMinSpeechDurationMs = durationMs;
    }

    void setMinSilenceDuration(uint64_t durationMs) {
        mMinSilenceDurationMs = durationMs;
    }
};

// VoiceActivityDetector implementation
VoiceActivityDetector::VoiceActivityDetector(int sampleRate, size_t frameSize)
    : mImpl(std::make_unique<Impl>(sampleRate, frameSize)) {
}

VoiceActivityDetector::~VoiceActivityDetector() = default;

bool VoiceActivityDetector::processAudioBuffer(const std::vector<short>& buffer) {
    return mImpl->processAudioBuffer(buffer);
}

bool VoiceActivityDetector::isSpeechActive() const {
    return mImpl->isSpeechActive();
}

uint64_t VoiceActivityDetector::getLastStateChangeTimestamp() const {
    return mImpl->getLastStateChangeTimestamp();
}

void VoiceActivityDetector::setSpeechEventCallback(const SpeechEventCallback& callback) {
    mImpl->setSpeechEventCallback(callback);
}

void VoiceActivityDetector::reset() {
    mImpl->reset();
}

void VoiceActivityDetector::setEnergyThreshold(double threshold) {
    mImpl->setEnergyThreshold(threshold);
}

void VoiceActivityDetector::setMinSpeechDuration(uint64_t durationMs) {
    mImpl->setMinSpeechDuration(durationMs);
}

void VoiceActivityDetector::setMinSilenceDuration(uint64_t durationMs) {
    mImpl->setMinSilenceDuration(durationMs);
}

} // namespace vad
