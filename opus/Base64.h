#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace utils {

/**
 * @brief Base64 encoding/decoding utility class for audio PCM data
 *
 * This class provides static methods to encode and decode vector of short values
 * (typically audio PCM data) to and from Base64 strings.
 *
 * Note: This implementation uses a custom approach where each 16-bit short
 * is encoded as exactly 3 Base64 characters, avoiding padding issues.
 */
class Base64 {
public:
    // Delete default constructor since all methods are static
    Base64() = delete;

    /**
     * @brief Encode vector of short values to Base64 string
     * @param data Vector of short values (audio PCM data)
     * @return Base64 encoded string
     */
    static std::string encode(const std::vector<short>& data);

    /**
     * @brief Decode Base64 string to vector of short values
     * @param encoded Base64 encoded string
     * @return Vector of decoded short values
     * @throws std::runtime_error if decoding fails
     */
    static std::vector<short> decode(const std::string& encoded);

    /**
     * @brief Encode raw short array to Base64 string
     * @param data Pointer to short array
     * @param size Number of elements in the array
     * @return Base64 encoded string
     */
    static std::string encode(const short* data, std::size_t size);

    /**
     * @brief Decode Base64 string to raw short array
     * @param encoded Base64 encoded string
     * @param[out] data Pointer to output short array (must be pre-allocated)
     * @param[out] size Number of decoded elements
     * @throws std::runtime_error if decoding fails
     */
    static void decode(const std::string& encoded, short* data, std::size_t& size);

    /**
     * @brief Check if a string is valid Base64
     * @param str String to validate
     * @return true if valid Base64, false otherwise
     */
    static bool isValid(const std::string& str);
};

} // namespace utils
