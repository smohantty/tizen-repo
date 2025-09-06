#include "Base64Helper.h"
#include "Base64.h"
#include <stdexcept>
#include <cstring>

namespace opus {

std::string Base64Helper::encode(const std::vector<unsigned char>& data) {
    if (data.empty()) {
        return "";
    }

    // Create a temporary vector of shorts to work with the existing Base64 implementation
    // We'll treat the unsigned char data as raw bytes and convert to shorts
    std::vector<short> shortData;

    // Ensure we have an even number of bytes
    size_t byteCount = data.size();
    if (byteCount % 2 != 0) {
        byteCount++; // We'll pad with zero
    }

    shortData.reserve(byteCount / 2);

    // Convert bytes to shorts (little-endian)
    for (size_t i = 0; i < data.size(); i += 2) {
        short value = 0;
        if (i < data.size()) {
            value |= static_cast<short>(data[i]);
        }
        if (i + 1 < data.size()) {
            value |= static_cast<short>(data[i + 1]) << 8;
        }
        shortData.push_back(value);
    }

    // If we had an odd number of bytes, add a zero short to complete the pair
    if (data.size() % 2 != 0) {
        shortData.push_back(0);
    }

    return utils::Base64::encode(shortData);
}

std::vector<unsigned char> Base64Helper::decode(const std::string& encoded) {
    if (encoded.empty()) {
        return {};
    }

    // Decode using existing Base64 implementation
    std::vector<short> shortData = utils::Base64::decode(encoded);

    // Convert shorts back to unsigned char vector
    std::vector<unsigned char> result;
    result.reserve(shortData.size() * 2);

    for (short value : shortData) {
        result.push_back(static_cast<unsigned char>(value & 0xFF));
        result.push_back(static_cast<unsigned char>((value >> 8) & 0xFF));
    }

    return result;
}

} // namespace opus
