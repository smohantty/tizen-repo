#include "JsonFormatter.h"
#include "AudioStreamingConfig.h"
#include "Message.h"
#include <random>
#include <sstream>

using json = nlohmann::json;

namespace edgeprocessor {

class JsonFormatter::Impl {
public:
    Impl() = default;
    ~Impl() = default;



    std::string formatStart(const AudioStreamingConfig& config) {
        json j;
        j["type"] = "start";
        j["session_id"] = config.sessionId.empty() ? generateUuid() : config.sessionId;
        j["format"]["sample_rate_hz"] = config.sampleRateHz;
        j["format"]["bits_per_sample"] = static_cast<int>(config.bitsPerSample);
        j["format"]["channels"] = static_cast<int>(config.channels);
        j["options"]["partial_results"] = true;
        j["options"]["compression"] = "pcm16";
        return j.dump();
    }

        std::string formatAudio(const std::vector<uint8_t>& pcmData,
                           uint64_t ptsMs,
                           uint32_t seq,
                           bool last) {
        json j;
        j["type"] = "audio";
        j["seq"] = seq;
        j["pts_ms"] = ptsMs;
        j["last"] = last;
        j["payload"] = encodeBase64(pcmData);
        return j.dump();
    }

    std::string formatEnd(uint32_t seq) {
        json j;
        j["type"] = "end";
        j["seq"] = seq;
        j["last"] = true;
        return j.dump();
    }

    EvPartial parsePartial(const std::string& jsonStr) {
        try {
            json j = json::parse(jsonStr);
            EvPartial result;
            result.text = j.value("text", "");
            result.stability = j.value("stability", 0.0f);
            return result;
        } catch (const json::exception& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()));
        }
    }

        EvFinal parseFinal(const std::string& jsonStr) {
        try {
            json j = json::parse(jsonStr);
            EvFinal result;
            result.text = j.value("text", "");
            result.confidence = j.value("confidence", 0.0f);
            return result;
        } catch (const json::exception& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()));
        }
    }

        EvLatency parseLatency(const std::string& jsonStr) {
        try {
            json j = json::parse(jsonStr);
            EvLatency result;
            result.upstreamMs = j.value("upstream_ms", 0u);
            result.e2eMs = j.value("e2e_ms", 0u);
            return result;
        } catch (const json::exception& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()));
        }
    }

        EvStatus parseStatus(const std::string& jsonStr) {
        try {
            json j = json::parse(jsonStr);
            EvStatus result;
            result.message = j.value("message", "");
            return result;
        } catch (const json::exception& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()));
        }
    }

        EvError parseError(const std::string& jsonStr) {
        try {
            json j = json::parse(jsonStr);
            EvError result;
            result.what = j.value("error", "");
            return result;
        } catch (const json::exception& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()));
        }
    }

    static std::string encodeBase64(const std::vector<uint8_t>& data) {
        static const char base64Chars[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string result;
        result.reserve(((data.size() + 2) / 3) * 4);

        for (size_t i = 0; i < data.size(); i += 3) {
            uint32_t chunk = 0;
            chunk |= static_cast<uint32_t>(data[i]) << 16;

            if (i + 1 < data.size()) {
                chunk |= static_cast<uint32_t>(data[i + 1]) << 8;
            }
            if (i + 2 < data.size()) {
                chunk |= static_cast<uint32_t>(data[i + 2]);
            }

            result += base64Chars[(chunk >> 18) & 0x3F];
            result += base64Chars[(chunk >> 12) & 0x3F];
            result += base64Chars[(chunk >> 6) & 0x3F];
            result += base64Chars[chunk & 0x3F];
        }

        // Add padding
        size_t padding = (3 - (data.size() % 3)) % 3;
        for (size_t i = 0; i < padding; ++i) {
            result[result.size() - 1 - i] = '=';
        }

        return result;
    }

    static std::vector<uint8_t> decodeBase64(const std::string& base64) {
        static const uint8_t base64Decode[] = {
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
            64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
            64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
        };

        std::vector<uint8_t> result;
        result.reserve((base64.size() / 4) * 3);

        for (size_t i = 0; i < base64.size(); i += 4) {
            uint32_t chunk = 0;
            int validChars = 0;

            for (int j = 0; j < 4 && i + j < base64.size(); ++j) {
                char c = base64[i + j];
                if (c == '=') break;

                if (static_cast<uint8_t>(c) >= 128 || base64Decode[static_cast<uint8_t>(c)] == 64) {
                    throw std::runtime_error("Invalid Base64 character");
                }

                chunk |= static_cast<uint32_t>(base64Decode[static_cast<uint8_t>(c)]) << (18 - j * 6);
                validChars++;
            }

            if (validChars >= 2) {
                result.push_back(static_cast<uint8_t>((chunk >> 16) & 0xFF));
            }
            if (validChars >= 3) {
                result.push_back(static_cast<uint8_t>((chunk >> 8) & 0xFF));
            }
            if (validChars >= 4) {
                result.push_back(static_cast<uint8_t>(chunk & 0xFF));
            }
        }

        return result;
    }

    static std::string generateUuid() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        static std::uniform_int_distribution<> dis2(8, 11);

        std::ostringstream uuid;
        uuid << std::hex << std::setfill('0');

        for (int i = 0; i < 8; ++i) {
            uuid << std::setw(1) << dis(gen);
        }
        uuid << "-";

        for (int i = 0; i < 4; ++i) {
            uuid << std::setw(1) << dis(gen);
        }
        uuid << "-";

        uuid << std::setw(1) << dis2(gen);
        for (int i = 0; i < 3; ++i) {
            uuid << std::setw(1) << dis(gen);
        }
        uuid << "-";

        for (int i = 0; i < 4; ++i) {
            uuid << std::setw(1) << dis(gen);
        }
        uuid << "-";

        for (int i = 0; i < 12; ++i) {
            uuid << std::setw(1) << dis(gen);
        }

        return uuid.str();
    }
};

// JsonFormatter implementation
JsonFormatter::JsonFormatter()
    : mPimpl(std::make_unique<Impl>()) {
}

JsonFormatter::~JsonFormatter() = default;

std::string JsonFormatter::formatStart(const AudioStreamingConfig& config) {
    return mPimpl->formatStart(config);
}

std::string JsonFormatter::formatAudio(const std::vector<uint8_t>& pcmData,
                                      uint64_t ptsMs,
                                      uint32_t seq,
                                      bool last) {
    return mPimpl->formatAudio(pcmData, ptsMs, seq, last);
}

std::string JsonFormatter::formatEnd(uint32_t seq) {
    return mPimpl->formatEnd(seq);
}

EvPartial JsonFormatter::parsePartial(const std::string& json) {
    return mPimpl->parsePartial(json);
}

EvFinal JsonFormatter::parseFinal(const std::string& json) {
    return mPimpl->parseFinal(json);
}

EvLatency JsonFormatter::parseLatency(const std::string& json) {
    return mPimpl->parseLatency(json);
}

EvStatus JsonFormatter::parseStatus(const std::string& json) {
    return mPimpl->parseStatus(json);
}

EvError JsonFormatter::parseError(const std::string& json) {
    return mPimpl->parseError(json);
}

std::string JsonFormatter::encodeBase64(const std::vector<uint8_t>& data) {
    return Impl::encodeBase64(data);
}

std::vector<uint8_t> JsonFormatter::decodeBase64(const std::string& base64) {
    return Impl::decodeBase64(base64);
}

std::string JsonFormatter::generateUuid() {
    return Impl::generateUuid();
}

} // namespace edgeprocessor
