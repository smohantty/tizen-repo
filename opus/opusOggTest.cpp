#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <opus.h>
#include <sndfile.h>
#include <ogg/ogg.h>

namespace opus {

/**
 * @brief Simple WAV file reader using libsndfile
 */
class WavReader {
public:
    WavReader(const std::string& filename) {
        SF_INFO sfinfo;
        std::memset(&sfinfo, 0, sizeof(sfinfo));

        mFile = sf_open(filename.c_str(), SFM_READ, &sfinfo);
        if (!mFile) {
            throw std::runtime_error("Failed to open WAV file: " + filename);
        }

        mSampleRate = sfinfo.samplerate;
        mChannels = sfinfo.channels;
        mTotalSamples = sfinfo.frames;

        std::cout << "Opened WAV file: " << filename << "\n";
        std::cout << "  Sample rate: " << mSampleRate << " Hz\n";
        std::cout << "  Channels: " << mChannels << "\n";
        std::cout << "  Total samples: " << mTotalSamples << "\n";
    }

    ~WavReader() {
        if (mFile) {
            sf_close(mFile);
        }
    }

    std::vector<opus_int16> readAll() {
        std::vector<opus_int16> samples(mTotalSamples * mChannels);
        sf_count_t read = sf_read_short(mFile, samples.data(), samples.size());
        samples.resize(read);
        return samples;
    }

    int getSampleRate() const { return mSampleRate; }
    int getChannels() const { return mChannels; }
    sf_count_t getTotalSamples() const { return mTotalSamples; }

private:
    SNDFILE* mFile;
    int mSampleRate;
    int mChannels;
    sf_count_t mTotalSamples;
};

/**
 * @brief Opus OGG encoder - encodes PCM to Opus and writes to OGG container
 */
class OpusOggEncoder {
public:
    OpusOggEncoder(const std::string& outputFile, int sampleRate, int channels, int application = OPUS_APPLICATION_AUDIO)
        : mSampleRate(sampleRate)
        , mChannels(channels)
        , mFrameSize((sampleRate * 20) / 1000) // 20ms frame
        , mEncoder(nullptr)
        , mPacketCount(0)
        , mGranulePos(0)
    {
        int error = 0;
        mEncoder = opus_encoder_create(sampleRate, channels, application, &error);
        if (error != OPUS_OK || !mEncoder) {
            throw std::runtime_error("Failed to create Opus encoder: " + std::string(opus_strerror(error)));
        }

        // Set encoder parameters
        opus_encoder_ctl(mEncoder, OPUS_SET_BITRATE(128000)); // 128 kbps
        opus_encoder_ctl(mEncoder, OPUS_SET_VBR(1)); // Variable bitrate
        opus_encoder_ctl(mEncoder, OPUS_SET_COMPLEXITY(10)); // Max quality

        // Open output file
        mOutFile.open(outputFile, std::ios::binary);
        if (!mOutFile.is_open()) {
            opus_encoder_destroy(mEncoder);
            throw std::runtime_error("Failed to open output file: " + outputFile);
        }

        // Initialize OGG stream
        ogg_stream_init(&mStreamState, rand());

        // Write OGG Opus header
        writeOpusHeader();
        writeOpusComments();
    }

    ~OpusOggEncoder() {
        if (mEncoder) {
            finalize();
            opus_encoder_destroy(mEncoder);
        }
        ogg_stream_clear(&mStreamState);
        if (mOutFile.is_open()) {
            mOutFile.close();
        }
    }

    void encode(const std::vector<opus_int16>& pcmData) {
        size_t samplesProcessed = 0;
        std::vector<unsigned char> compressedFrame(4000); // Max Opus frame size

        while (samplesProcessed < pcmData.size()) {
            size_t samplesRemaining = pcmData.size() - samplesProcessed;
            size_t samplesToEncode = std::min(static_cast<size_t>(mFrameSize * mChannels), samplesRemaining);

            // Pad with zeros if we don't have a complete frame
            std::vector<opus_int16> frame(mFrameSize * mChannels, 0);
            std::copy(pcmData.begin() + samplesProcessed,
                     pcmData.begin() + samplesProcessed + samplesToEncode,
                     frame.begin());

            // Encode frame
            int compressedSize = opus_encode(mEncoder,
                                            frame.data(),
                                            mFrameSize,
                                            compressedFrame.data(),
                                            compressedFrame.size());

            if (compressedSize < 0) {
                throw std::runtime_error("Opus encoding failed: " + std::string(opus_strerror(compressedSize)));
            }

            if (compressedSize > 0) {
                // Write to OGG stream
                writeOpusPacket(compressedFrame.data(), compressedSize);
            }

            samplesProcessed += mFrameSize * mChannels;
        }
    }

    void finalize() {
        // Create an end-of-stream packet
        ogg_packet eosPacket;
        unsigned char dummy = 0;
        eosPacket.packet = &dummy;
        eosPacket.bytes = 0;
        eosPacket.b_o_s = 0;
        eosPacket.e_o_s = 1; // End of stream
        eosPacket.granulepos = mGranulePos;
        eosPacket.packetno = mPacketCount++;

        ogg_stream_packetin(&mStreamState, &eosPacket);

        // Flush remaining OGG pages
        ogg_page page;
        while (ogg_stream_flush(&mStreamState, &page)) {
            mOutFile.write(reinterpret_cast<char*>(page.header), page.header_len);
            mOutFile.write(reinterpret_cast<char*>(page.body), page.body_len);
        }
    }

private:
    void writeOpusHeader() {
        // OpusHead packet
        unsigned char header[19];
        std::memcpy(header, "OpusHead", 8);
        header[8] = 1; // Version
        header[9] = mChannels;
        // Pre-skip: 3840 samples (80ms at 48kHz) - standard for Opus
        uint16_t preskip = 3840;
        header[10] = preskip & 0xFF; // Pre-skip LSB
        header[11] = (preskip >> 8) & 0xFF; // Pre-skip MSB
        header[12] = mSampleRate & 0xFF;
        header[13] = (mSampleRate >> 8) & 0xFF;
        header[14] = (mSampleRate >> 16) & 0xFF;
        header[15] = (mSampleRate >> 24) & 0xFF;
        header[16] = 0; // Output gain LSB
        header[17] = 0; // Output gain MSB
        header[18] = 0; // Channel mapping family

        ogg_packet packet;
        packet.packet = header;
        packet.bytes = 19;
        packet.b_o_s = 1; // Beginning of stream
        packet.e_o_s = 0;
        packet.granulepos = 0;
        packet.packetno = mPacketCount++;

        ogg_stream_packetin(&mStreamState, &packet);

        // Write out the header page
        ogg_page page;
        while (ogg_stream_flush(&mStreamState, &page)) {
            mOutFile.write(reinterpret_cast<char*>(page.header), page.header_len);
            mOutFile.write(reinterpret_cast<char*>(page.body), page.body_len);
        }
    }

    void writeOpusComments() {
        // OpusTags packet
        std::string vendor = "opus-test-encoder";
        std::vector<unsigned char> comments(8 + 4 + vendor.length() + 4);

        std::memcpy(comments.data(), "OpusTags", 8);
        size_t pos = 8;

        // Vendor string length
        uint32_t vendorLen = vendor.length();
        std::memcpy(comments.data() + pos, &vendorLen, 4);
        pos += 4;

        // Vendor string
        std::memcpy(comments.data() + pos, vendor.c_str(), vendor.length());
        pos += vendor.length();

        // User comment list length (0)
        uint32_t commentCount = 0;
        std::memcpy(comments.data() + pos, &commentCount, 4);

        ogg_packet packet;
        packet.packet = comments.data();
        packet.bytes = comments.size();
        packet.b_o_s = 0;
        packet.e_o_s = 0;
        packet.granulepos = 0;
        packet.packetno = mPacketCount++;

        ogg_stream_packetin(&mStreamState, &packet);

        // Write out the comments page
        ogg_page page;
        while (ogg_stream_flush(&mStreamState, &page)) {
            mOutFile.write(reinterpret_cast<char*>(page.header), page.header_len);
            mOutFile.write(reinterpret_cast<char*>(page.body), page.body_len);
        }
    }

    void writeOpusPacket(const unsigned char* data, int size) {
        // Opus granule positions are always in 48kHz units
        // Scale the frame size to 48kHz
        long long samplesAt48kHz = (mFrameSize * 48000LL) / mSampleRate;
        mGranulePos += samplesAt48kHz;

        ogg_packet packet;
        packet.packet = const_cast<unsigned char*>(data);
        packet.bytes = size;
        packet.b_o_s = 0;
        packet.e_o_s = 0;
        packet.granulepos = mGranulePos;
        packet.packetno = mPacketCount++;

        ogg_stream_packetin(&mStreamState, &packet);

        // Write out pages as they become available
        ogg_page page;
        while (ogg_stream_pageout(&mStreamState, &page)) {
            mOutFile.write(reinterpret_cast<char*>(page.header), page.header_len);
            mOutFile.write(reinterpret_cast<char*>(page.body), page.body_len);
        }
    }

    int mSampleRate;
    int mChannels;
    int mFrameSize;
    OpusEncoder* mEncoder;
    ogg_stream_state mStreamState;
    std::ofstream mOutFile;
    long long mPacketCount;
    long long mGranulePos;
};

} // namespace opus

int main(int argc, char* argv[]) {
    try {
        std::string inputFile = "input.wav";
        std::string outputFile = "output.opus";

        if (argc > 1) {
            inputFile = argv[1];
        }
        if (argc > 2) {
            outputFile = argv[2];
        }

        std::cout << "=== Opus OGG Compression Test ===\n";
        std::cout << "Input WAV file: " << inputFile << "\n";
        std::cout << "Output OGG file: " << outputFile << "\n\n";

        // Read WAV file
        opus::WavReader reader(inputFile);
        auto pcmData = reader.readAll();

        std::cout << "Read " << pcmData.size() << " samples from WAV file\n";
        std::cout << "Duration: " << static_cast<double>(pcmData.size()) / reader.getChannels() / reader.getSampleRate() << " seconds\n\n";

        // Encode to Opus in OGG container
        std::cout << "Encoding to Opus...\n";
        opus::OpusOggEncoder encoder(outputFile, reader.getSampleRate(), reader.getChannels());
        encoder.encode(pcmData);

        std::cout << "\nEncoding complete!\n";
        std::cout << "Output saved to: " << outputFile << "\n";

        // Calculate compression ratio
        std::ifstream inFile(inputFile, std::ios::binary | std::ios::ate);
        std::ifstream outFile(outputFile, std::ios::binary | std::ios::ate);

        if (inFile.is_open() && outFile.is_open()) {
            size_t inSize = inFile.tellg();
            size_t outSize = outFile.tellg();
            double compressionRatio = static_cast<double>(inSize) / outSize;

            std::cout << "\nCompression Statistics:\n";
            std::cout << "  Input size:  " << inSize << " bytes\n";
            std::cout << "  Output size: " << outSize << " bytes\n";
            std::cout << "  Compression ratio: " << compressionRatio << ":1\n";
        }

        std::cout << "\nSUCCESS: WAV file compressed to Opus OGG format!\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}

