#include "OpusAudioCodec.h"
#include <stdexcept>
#include <iostream>

namespace opus {

class OpusAudioCodec::Impl {
public:
    Impl(int sampleRate, int channels, int application)
        : mSampleRate(sampleRate)
        , mChannels(channels)
        , mFrameSize((sampleRate * 20) / 1000) // 20ms frame size based on sample rate
        , mEncoder(nullptr)
        , mDecoder(nullptr)
    {
        int error = 0;

        // Create encoder
        mEncoder = opus_encoder_create(sampleRate, channels, application, &error);
        if (error != OPUS_OK || !mEncoder) {
            throw std::runtime_error("Failed to create Opus encoder: " + std::string(opus_strerror(error)));
        }

        // Create decoder
        mDecoder = opus_decoder_create(sampleRate, channels, &error);
        if (error != OPUS_OK || !mDecoder) {
            opus_encoder_destroy(mEncoder);
            throw std::runtime_error("Failed to create Opus decoder: " + std::string(opus_strerror(error)));
        }

        // Encoder and decoder created successfully

        // Set encoder parameters
        opus_encoder_ctl(mEncoder, OPUS_SET_BITRATE(64000)); // 64 kbps
        opus_encoder_ctl(mEncoder, OPUS_SET_VBR(1)); // Variable bitrate
        opus_encoder_ctl(mEncoder, OPUS_SET_COMPLEXITY(5)); // Medium complexity

        // Warm up the encoder/decoder with multiple dummy frames
        for (int i = 0; i < 2; i++) {
            std::vector<opus_int16> warmupFrame(mFrameSize, i * 100);
            unsigned char warmupCompressed[4000];
            int warmupSize = opus_encode(mEncoder, warmupFrame.data(), mFrameSize, warmupCompressed, sizeof(warmupCompressed));
            if (warmupSize > 0) {
                std::vector<opus_int16> warmupDecoded(mFrameSize);
                int decoded = opus_decode(mDecoder, warmupCompressed, warmupSize, warmupDecoded.data(), mFrameSize, 0);
                // Warmup frame processed
            }
        }
    }

    ~Impl() {
        if (mEncoder) {
            opus_encoder_destroy(mEncoder);
        }
        if (mDecoder) {
            opus_decoder_destroy(mDecoder);
        }
    }

    std::vector<unsigned char> encode(const std::vector<opus_int16>& pcm) {
        if (pcm.empty()) {
            return {};
        }

        // Ensure we have the right frame size
        if (pcm.size() != static_cast<size_t>(mFrameSize)) {
            throw std::runtime_error("PCM frame size mismatch. Expected: " +
                                   std::to_string(mFrameSize) +
                                   ", Got: " + std::to_string(pcm.size()));
        }

        // Maximum possible output size for a frame
        const int maxDataBytes = 4000;
        std::vector<unsigned char> compressed(maxDataBytes);

        int compressedSize = opus_encode(mEncoder, pcm.data(), mFrameSize,
                                       compressed.data(), maxDataBytes);

        if (compressedSize < 0) {
            throw std::runtime_error("Opus encoding failed: " + std::string(opus_strerror(compressedSize)));
        }

        // Resize to actual compressed size
        compressed.resize(compressedSize);
        return compressed;
    }

    std::vector<opus_int16> decode(const std::vector<unsigned char>& data) {
        if (data.empty()) {
            return {};
        }

        std::vector<opus_int16> pcm(mFrameSize);

        int decodedSamples = opus_decode(mDecoder, data.data(), data.size(),
                                       pcm.data(), mFrameSize, 0);

        // Decoding completed

        if (decodedSamples < 0) {
            throw std::runtime_error("Opus decoding failed: " + std::string(opus_strerror(decodedSamples)));
        }

        // Opus decoder might return fewer samples than requested, which is normal
        if (decodedSamples > 0 && decodedSamples <= mFrameSize) {
            pcm.resize(decodedSamples);
        } else if (decodedSamples == 0) {
            // This might indicate silence or an issue
            std::cerr << "Warning: Opus decoder returned 0 samples" << std::endl;
        }

        return pcm;
    }

    int getFrameSize() const { return mFrameSize; }
    int getSampleRate() const { return mSampleRate; }
    int getChannels() const { return mChannels; }

private:
    int mSampleRate;
    int mChannels;
    int mFrameSize;
    OpusEncoder* mEncoder;
    OpusDecoder* mDecoder;
};

// OpusAudioCodec implementation
OpusAudioCodec::OpusAudioCodec(int sampleRate, int channels, int application)
    : mImpl(std::make_unique<Impl>(sampleRate, channels, application)) {}

OpusAudioCodec::~OpusAudioCodec() = default;

std::vector<unsigned char> OpusAudioCodec::encode(const std::vector<opus_int16>& pcm) {
    return mImpl->encode(pcm);
}

std::vector<opus_int16> OpusAudioCodec::decode(const std::vector<unsigned char>& data) {
    return mImpl->decode(data);
}

int OpusAudioCodec::getFrameSize() const {
    return mImpl->getFrameSize();
}

int OpusAudioCodec::getSampleRate() const {
    return mImpl->getSampleRate();
}

int OpusAudioCodec::getChannels() const {
    return mImpl->getChannels();
}

} // namespace opus
