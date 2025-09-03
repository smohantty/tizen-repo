#include "Base64.h"
#include <stdexcept>
#include <algorithm>
#include <cstring> // For strchr

namespace utils {

namespace {
    // Base64 character set
    constexpr const char* BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // Helper function to find character in Base64 set
    inline int findChar(char c) {
        const char* pos = strchr(BASE64_CHARS, c);
        return pos ? static_cast<int>(pos - BASE64_CHARS) : -1;
    }

    // Helper function to check if character is valid Base64
    inline bool isValidChar(char c) {
        return strchr(BASE64_CHARS, c) != nullptr;
    }
}

std::string Base64::encode(const short* data, std::size_t size) {
    if (!data || size == 0) {
        return "";
    }

    // For simplicity, encode each short as a 16-bit value
    // Each short becomes exactly 3 Base64 characters
    std::string result;
    result.reserve(size * 3);

    for (std::size_t i = 0; i < size; ++i) {
        // Convert short to 3 Base64 characters
        uint16_t value = static_cast<uint16_t>(data[i]);

        char c1 = BASE64_CHARS[(value >> 12) & 0x3F];
        char c2 = BASE64_CHARS[(value >> 6) & 0x3F];
        char c3 = BASE64_CHARS[value & 0x3F];

        result += c1;
        result += c2;
        result += c3;
    }

    return result;
}

std::vector<short> Base64::decode(const std::string& encoded) {
    if (encoded.empty()) {
        return {};
    }

    // Validate input length (must be multiple of 3)
    if (encoded.length() % 3 != 0) {
        throw std::runtime_error("Invalid Base64 string length");
    }

    std::vector<short> result;
    result.reserve(encoded.length() / 3);

    // Process each group of 3 characters
    for (size_t i = 0; i < encoded.length(); i += 3) {
        if (i + 2 >= encoded.length()) break; // Should not happen with length check

        int v1 = findChar(encoded[i]);
        int v2 = findChar(encoded[i + 1]);
        int v3 = findChar(encoded[i + 2]);

        if (v1 == -1 || v2 == -1 || v3 == -1) {
            throw std::runtime_error("Invalid Base64 character");
        }

        // Reconstruct the 16-bit value
        uint16_t value = (v1 << 12) | (v2 << 6) | v3;
        result.push_back(static_cast<short>(value));
    }

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
    if (str.empty()) {
        return true;
    }

    // Check length (must be multiple of 3)
    if (str.length() % 3 != 0) {
        return false;
    }

    // Check characters
    for (char c : str) {
        if (!isValidChar(c)) {
            return false;
        }
    }

    return true;
}

// Public interface implementation
std::string Base64::encode(const std::vector<short>& data) {
    return encode(data.data(), data.size());
}

} // namespace utils
