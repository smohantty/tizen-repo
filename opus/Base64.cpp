#include "Base64.h"
#include <stdexcept>
#include <algorithm>
#include <cstring> // For strchr

namespace utils {

namespace {
    constexpr const char* BASE64_CHARS =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    inline int findChar(char c) {
        const char* pos = strchr(BASE64_CHARS, c);
        return pos ? static_cast<int>(pos - BASE64_CHARS) : -1;
    }

    inline bool isValidChar(char c) {
        return (c == '=') || (strchr(BASE64_CHARS, c) != nullptr);
    }
}

std::string Base64::encode(const short* data, std::size_t size) {
    if (!data || size == 0) {
        return "";
    }

    // reinterpret shorts as raw bytes
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(data);
    std::size_t byteLen = size * sizeof(short);

    std::string result;
    result.reserve(((byteLen + 2) / 3) * 4);

    for (std::size_t i = 0; i < byteLen; i += 3) {
        uint32_t triple = 0;
        int count = std::min<std::size_t>(3, byteLen - i);

        for (int j = 0; j < count; ++j) {
            triple |= (bytes[i + j] << ((2 - j) * 8));
        }

        for (int j = 0; j < 4; ++j) {
            if (j <= (count + 0)) {
                result.push_back(BASE64_CHARS[(triple >> ((3 - j) * 6)) & 0x3F]);
            } else {
                result.push_back('=');
            }
        }
    }

    return result;
}

std::vector<short> Base64::decode(const std::string& encoded) {
    if (encoded.empty()) {
        return {};
    }

    if (encoded.length() % 4 != 0) {
        throw std::runtime_error("Invalid Base64 string length");
    }

    std::vector<unsigned char> bytes;
    bytes.reserve((encoded.length() / 4) * 3);

    for (std::size_t i = 0; i < encoded.length(); i += 4) {
        uint32_t triple = 0;
        int pad = 0;

        for (int j = 0; j < 4; ++j) {
            char c = encoded[i + j];
            if (c == '=') {
                triple <<= 6;
                ++pad;
            } else {
                int v = findChar(c);
                if (v == -1) {
                    throw std::runtime_error("Invalid Base64 character");
                }
                triple = (triple << 6) | v;
            }
        }

        for (int j = 0; j < 3 - pad; ++j) {
            bytes.push_back((triple >> ((2 - j) * 8)) & 0xFF);
        }
    }

    if (bytes.size() % sizeof(short) != 0) {
        throw std::runtime_error("Decoded byte count not aligned to short");
    }

    std::vector<short> result(bytes.size() / sizeof(short));
    std::memcpy(result.data(), bytes.data(), bytes.size());

    return result;
}

void Base64::decode(const std::string& encoded, short* data, std::size_t& size) {
    std::vector<short> decoded = decode(encoded);
    if (decoded.size() > size) {
        throw std::runtime_error("Output buffer too small");
    }
    std::copy(decoded.begin(), decoded.end(), data);
    size = decoded.size();
}

bool Base64::isValid(const std::string& str) {
    if (str.empty()) return true;
    if (str.length() % 4 != 0) return false;

    for (char c : str) {
        if (!isValidChar(c)) {
            return false;
        }
    }
    return true;
}

std::string Base64::encode(const std::vector<short>& data) {
    return encode(data.data(), data.size());
}

} // namespace utils
