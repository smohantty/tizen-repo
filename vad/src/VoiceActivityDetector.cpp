#include "VoiceActivityDetector.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>
#include <stdexcept>

namespace vad {

using SpeechState = vad::SpeechState;

namespace {
    // Default configuration constants
    constexpr float kDefaultSpeechThreshold = 0.5f;
    constexpr uint64_t kDefaultMinSpeechDurationMs = 100;
    constexpr uint64_t kDefaultMinSilenceDurationMs = 200;
    constexpr uint64_t kDefaultPrerollDurationMs = 500;
    constexpr size_t kSmoothingWindowFrames = 10; // 100ms window - better for ASR streaming
    constexpr float kExponentialSmoothingAlpha = 0.2f; // Decay factor for exponential averaging

    // TensorFlow Lite model requirements
    constexpr size_t kTensorFlowLiteFrameSize = 160; // 10ms at 16kHz as required by model
}

// Mock TensorFlow Lite model class
class MockTensorFlowLiteModel {
public:
    explicit MockTensorFlowLiteModel(const std::string& modelPath) {
        // In a real implementation, this would load the TensorFlow Lite model
        // For now, just store the path and simulate successful loading
        mModelPath = modelPath;
        mIsLoaded = true;
    }

    float predict(const std::vector<short>& audioFrame) {
        // Mock implementation that returns a random-ish probability
        // In real implementation, this would:
        // 1. Convert audio frame to the format expected by the model
        // 2. Run inference using TensorFlow Lite interpreter
        // 3. Return the speech probability (0.0 to 1.0)

        // Simple mock: calculate energy and map to probability
        double energy = 0.0;
        for (short sample : audioFrame) {
            energy += static_cast<double>(sample) * sample;
        }
        energy = std::sqrt(energy / audioFrame.size());

        // Map energy to probability (this is just a mockup)
        float probability = std::min(1.0f, static_cast<float>(energy / 10000.0));
        return probability;
    }

    bool isLoaded() const { return mIsLoaded; }

private:
    std::string mModelPath;
    bool mIsLoaded = false;
};

class VoiceActivityDetector::Impl {
public:
    // Configuration
    int mSampleRate;
    size_t mFrameSize;
    float mSpeechThreshold;
    uint64_t mMinSpeechDurationMs;
    uint64_t mMinSilenceDurationMs;

    // State
    std::vector<short> mBuffer;
    std::vector<short> mPrerollBuffer;
    bool mIsSpeechActive;
    uint64_t mCurrentTimestamp;
    uint64_t mSpeechStartTimestamp;
    uint64_t mSilenceStartTimestamp;

    // Smoothing
    std::vector<float> mRecentProbabilities;
    size_t mProbabilityIndex;

    // Exponential smoothing
    float mExponentialAverage;
    bool mExponentialAverageInitialized;

    // TensorFlow Lite model
    std::unique_ptr<MockTensorFlowLiteModel> mModel;

    // Callback
    SpeechEventCallback mCallback;

    explicit Impl(const std::string& modelPath, int sampleRate)
        : mSampleRate(sampleRate)
        , mFrameSize(kTensorFlowLiteFrameSize) // Fixed frame size required by TensorFlow Lite model
        , mSpeechThreshold(kDefaultSpeechThreshold)
        , mMinSpeechDurationMs(kDefaultMinSpeechDurationMs)
        , mMinSilenceDurationMs(kDefaultMinSilenceDurationMs)
        , mIsSpeechActive(false)
        , mCurrentTimestamp(0)
        , mSpeechStartTimestamp(0)
        , mSilenceStartTimestamp(0)
        , mProbabilityIndex(0)
        , mExponentialAverage(0.0f)
        , mExponentialAverageInitialized(false)
    {
        mRecentProbabilities.resize(kSmoothingWindowFrames, 0.0f);

        // Initialize TensorFlow Lite model
        mModel = std::make_unique<MockTensorFlowLiteModel>(modelPath);
        if (!mModel->isLoaded()) {
            throw std::runtime_error("Failed to load TensorFlow Lite model: " + modelPath);
        }

        // Calculate preroll buffer size with fixed duration
        size_t prerollSamples = (kDefaultPrerollDurationMs * mSampleRate) / 1000;
        mPrerollBuffer.reserve(prerollSamples);
    }

    void process(std::vector<short> audiobuff) {
        // Add new audio to buffer
        mBuffer.insert(mBuffer.end(), audiobuff.begin(), audiobuff.end());

        // Update preroll buffer
        updatePrerollBuffer(audiobuff);

        // Process complete frames
        while (mBuffer.size() >= mFrameSize) {
            std::vector<short> frame(mBuffer.begin(), mBuffer.begin() + mFrameSize);

            // Use TensorFlow Lite model to get speech probability
            float speechProbability = mModel->predict(frame);

            // Apply smoothing - choose one method for testing:
            bool smoothedDetection = updateSmoothing(speechProbability);           // Simple moving average
            // bool smoothedDetection = updateSmoothingExponential(speechProbability); // Exponential moving average

            // Update speech state and trigger callbacks if needed
            updateSpeechState(smoothedDetection, frame);

            // Remove processed frame
            mBuffer.erase(mBuffer.begin(), mBuffer.begin() + mFrameSize);

            // Update timestamp (frame duration in milliseconds)
            mCurrentTimestamp += (mFrameSize * 1000) / mSampleRate;
        }
    }

private:
    void updatePrerollBuffer(const std::vector<short>& audiobuff) {
        // Add new audio to preroll buffer
        mPrerollBuffer.insert(mPrerollBuffer.end(), audiobuff.begin(), audiobuff.end());

        // Keep only the required preroll duration (fixed at 500ms)
        size_t maxPrerollSamples = (kDefaultPrerollDurationMs * mSampleRate) / 1000;
        if (mPrerollBuffer.size() > maxPrerollSamples) {
            size_t excessSamples = mPrerollBuffer.size() - maxPrerollSamples;
            mPrerollBuffer.erase(mPrerollBuffer.begin(), mPrerollBuffer.begin() + excessSamples);
        }
    }

    bool updateSmoothing(float speechProbability) {
        // Add current probability to smoothing window
        mRecentProbabilities[mProbabilityIndex] = speechProbability;
        mProbabilityIndex = (mProbabilityIndex + 1) % kSmoothingWindowFrames;

        // Calculate average probability over 100ms window (10 frames)
        float avgProbability = std::accumulate(mRecentProbabilities.begin(), mRecentProbabilities.end(), 0.0f) / kSmoothingWindowFrames;

        // Return true if average probability exceeds threshold
        return avgProbability > mSpeechThreshold;
    }

    bool updateSmoothingExponential(float speechProbability) {
        if (!mExponentialAverageInitialized) {
            // Initialize with first value
            mExponentialAverage = speechProbability;
            mExponentialAverageInitialized = true;
        } else {
            // Exponential moving average: EMA = α * new_value + (1-α) * old_EMA
            mExponentialAverage = kExponentialSmoothingAlpha * speechProbability +
                                 (1.0f - kExponentialSmoothingAlpha) * mExponentialAverage;
        }

        // Return true if exponential average exceeds threshold
        return mExponentialAverage > mSpeechThreshold;
    }

    void updateSpeechState(bool detected, const std::vector<short>& currentFrame) {
        if (!mIsSpeechActive && detected) {
            // Potential speech start
            if (mSpeechStartTimestamp == 0) {
                mSpeechStartTimestamp = mCurrentTimestamp;
            } else if (mCurrentTimestamp - mSpeechStartTimestamp >= mMinSpeechDurationMs) {
                // Speech confirmed after minimum duration - trigger START
                mIsSpeechActive = true;
                mSilenceStartTimestamp = 0;

                if (mCallback) {
                    // START: Provide preroll buffer for ASR initialization
                    mCallback(SpeechState::START, mPrerollBuffer, mCurrentTimestamp);
                }
            }
        } else if (mIsSpeechActive && detected) {
            // Speech is continuing - trigger CONTINUE
            mSilenceStartTimestamp = 0; // Reset silence tracking

            if (mCallback) {
                // CONTINUE: Provide current frame for streaming ASR
                mCallback(SpeechState::CONTINUE, currentFrame, mCurrentTimestamp);
            }
        } else if (mIsSpeechActive && !detected) {
            // Potential speech end
            if (mSilenceStartTimestamp == 0) {
                mSilenceStartTimestamp = mCurrentTimestamp;
            } else if (mCurrentTimestamp - mSilenceStartTimestamp >= mMinSilenceDurationMs) {
                // Silence confirmed after minimum duration - trigger END
                mIsSpeechActive = false;
                mSpeechStartTimestamp = 0;

                if (mCallback) {
                    // END: Provide empty buffer to signal ASR finalization
                    std::vector<short> emptyBuffer;
                    mCallback(SpeechState::END, emptyBuffer, mCurrentTimestamp);
                }
            }
        } else if (!detected) {
            // Reset speech start tracking if no detection
            mSpeechStartTimestamp = 0;
        }
    }

public:
    bool isSpeechActive() const {
        return mIsSpeechActive;
    }

    void setSpeechEventCallback(const SpeechEventCallback& callback) {
        mCallback = callback;
    }

    void reset() {
        mBuffer.clear();
        mPrerollBuffer.clear();
        mIsSpeechActive = false;
        mCurrentTimestamp = 0;
        mSpeechStartTimestamp = 0;
        mSilenceStartTimestamp = 0;
        mProbabilityIndex = 0;
        std::fill(mRecentProbabilities.begin(), mRecentProbabilities.end(), 0.0f);
        mExponentialAverage = 0.0f;
        mExponentialAverageInitialized = false;
    }

    void setSpeechThreshold(float threshold) {
        mSpeechThreshold = std::max(0.0f, std::min(1.0f, threshold));
    }

    void setMinSpeechDuration(uint64_t durationMs) {
        mMinSpeechDurationMs = durationMs;
    }

    void setMinSilenceDuration(uint64_t durationMs) {
        mMinSilenceDurationMs = durationMs;
    }
};

// VoiceActivityDetector implementation
VoiceActivityDetector::VoiceActivityDetector(const std::string& modelPath, int sampleRate)
    : mImpl(std::make_unique<Impl>(modelPath, sampleRate)) {
}

VoiceActivityDetector::~VoiceActivityDetector() = default;

void VoiceActivityDetector::process(std::vector<short> audiobuff) {
    mImpl->process(std::move(audiobuff));
}

bool VoiceActivityDetector::isSpeechActive() const {
    return mImpl->isSpeechActive();
}

void VoiceActivityDetector::setSpeechEventCallback(const SpeechEventCallback& callback) {
    mImpl->setSpeechEventCallback(callback);
}

void VoiceActivityDetector::reset() {
    mImpl->reset();
}

void VoiceActivityDetector::setSpeechThreshold(float threshold) {
    mImpl->setSpeechThreshold(threshold);
}

void VoiceActivityDetector::setMinSpeechDuration(uint64_t durationMs) {
    mImpl->setMinSpeechDuration(durationMs);
}

void VoiceActivityDetector::setMinSilenceDuration(uint64_t durationMs) {
    mImpl->setMinSilenceDuration(durationMs);
}



} // namespace vad
