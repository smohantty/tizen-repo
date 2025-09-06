#pragma once

#include <vector>
#include <string>

namespace opus {

/**
 * @brief Base64 helper class for encoding/decoding Opus data
 *
 * This class provides static methods to encode and decode vector of unsigned char
 * (typically Opus compressed data) to and from Base64 strings.
 */
class Base64Helper {
public:
    // Delete default constructor since all methods are static
    Base64Helper() = delete;

    /**
     * @brief Encode vector of unsigned char to Base64 string
     * @param data Vector of unsigned char (Opus compressed data)
     * @return Base64 encoded string
     */
    static std::string encode(const std::vector<unsigned char>& data);

    /**
     * @brief Decode Base64 string to vector of unsigned char
     * @param encoded Base64 encoded string
     * @return Vector of decoded unsigned char
     * @throws std::runtime_error if decoding fails
     */
    static std::vector<unsigned char> decode(const std::string& encoded);
};

} // namespace opus
