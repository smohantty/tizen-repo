#include "WaveHeader.h"
#include <fstream>
#include <sstream>

namespace utils {

WaveHeader::WaveHeader(uint16_t numChannels, uint32_t sampleRate,
                       uint16_t bitsPerSample, uint32_t numSamples) {
    // Initialize RIFF header
    std::memcpy(mRiffId, "RIFF", 4);
    mFileSize = 36; // Will be updated by updateComputedFields()
    std::memcpy(mWaveId, "WAVE", 4);

    // Initialize format chunk
    std::memcpy(mFmtId, "fmt ", 4);
    mFmtSize = 16; // PCM format chunk size
    mAudioFormat = 1; // PCM
    mNumChannels = numChannels;
    mSampleRate = sampleRate;
    mBitsPerSample = bitsPerSample;

    // Initialize data chunk
    std::memcpy(mDataId, "data", 4);
    mDataSize = 0; // Will be set by setNumSamples() or updateComputedFields()

    // Compute derived fields
    setNumSamples(numSamples);
}

bool WaveHeader::readFromBuffer(const char* buffer) {
    if (!buffer) {
        return false;
    }

    // Copy entire structure from buffer
    std::memcpy(this, buffer, sizeof(WaveHeader));

    // Validate the header
    return isValid();
}

void WaveHeader::writeToBuffer(char* buffer) const {
    if (!buffer) {
        return;
    }

    // Copy entire structure to buffer
    std::memcpy(buffer, this, sizeof(WaveHeader));
}

bool WaveHeader::readFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }

    file.read(reinterpret_cast<char*>(this), sizeof(WaveHeader));

    if (!file || file.gcount() != sizeof(WaveHeader)) {
        return false;
    }

    return isValid();
}

bool WaveHeader::writeToFile(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(this), sizeof(WaveHeader));

    return file.good();
}

bool WaveHeader::isValid() const {
    // Check RIFF header
    if (std::memcmp(mRiffId, "RIFF", 4) != 0) {
        return false;
    }

    if (std::memcmp(mWaveId, "WAVE", 4) != 0) {
        return false;
    }

    // Check format chunk
    if (std::memcmp(mFmtId, "fmt ", 4) != 0) {
        return false;
    }

    if (mFmtSize != 16) {
        return false;
    }

    if (mAudioFormat != 1) { // Must be PCM
        return false;
    }

    // Check data chunk
    if (std::memcmp(mDataId, "data", 4) != 0) {
        return false;
    }

    // Validate parameters
    if (mNumChannels == 0 || mNumChannels > 8) {
        return false;
    }

    if (mSampleRate == 0 || mSampleRate > 192000) {
        return false;
    }

    if (mBitsPerSample != 8 && mBitsPerSample != 16 &&
        mBitsPerSample != 24 && mBitsPerSample != 32) {
        return false;
    }

    return true;
}

void WaveHeader::updateComputedFields() {
    mByteRate = mSampleRate * mNumChannels * (mBitsPerSample / 8);
    mBlockAlign = mNumChannels * (mBitsPerSample / 8);
    mFileSize = 36 + mDataSize;
}

uint32_t WaveHeader::getNumSamples() const {
    if (mNumChannels == 0 || mBitsPerSample == 0) {
        return 0;
    }
    return mDataSize / (mNumChannels * (mBitsPerSample / 8));
}

void WaveHeader::setNumSamples(uint32_t numSamples) {
    mDataSize = numSamples * mNumChannels * (mBitsPerSample / 8);
    updateComputedFields();
}

double WaveHeader::getDuration() const {
    if (mSampleRate == 0) {
        return 0.0;
    }
    return static_cast<double>(getNumSamples()) / mSampleRate;
}

std::string WaveHeader::getDescription() const {
    std::ostringstream oss;
    oss << "WAV Format:\n"
        << "  Channels: " << mNumChannels << "\n"
        << "  Sample Rate: " << mSampleRate << " Hz\n"
        << "  Bits Per Sample: " << mBitsPerSample << "\n"
        << "  Byte Rate: " << mByteRate << " bytes/sec\n"
        << "  Block Align: " << mBlockAlign << " bytes\n"
        << "  Number of Samples: " << getNumSamples() << "\n"
        << "  Data Size: " << mDataSize << " bytes\n"
        << "  Duration: " << getDuration() << " seconds";
    return oss.str();
}

} // namespace utils
