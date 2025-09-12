#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <type_traits>
#include <cstring>
#include <stdexcept>
#include <memory>

namespace utils {

/**
 * @brief Header-only serialization/deserialization library for binary file I/O
 *
 * This library provides utilities to serialize and deserialize:
 * - POD (Plain Old Data) types
 * - User-defined structs (that implement serialize/deserialize methods)
 * - Vectors of POD types and user-defined structs
 *
 * Usage examples:
 *
 * For POD types:
 * @code
 * int value = 42;
 * serde::saveToFile("data.bin", value);
 * int loaded = serde::loadFromFile<int>("data.bin");
 * @endcode
 *
 * For user-defined structs:
 * @code
 * struct MyStruct {
 *     int x;
 *     float y;
 *     std::string name;
 *
 *     void serialize(std::vector<uint8_t>& buffer) const {
 *         serde::write(buffer, x);
 *         serde::write(buffer, y);
 *         serde::write(buffer, name);
 *     }
 *
 *     void deserialize(const std::vector<uint8_t>& buffer, std::size_t& offset) {
 *         serde::read(buffer, offset, x);
 *         serde::read(buffer, offset, y);
 *         serde::read(buffer, offset, name);
 *     }
 * };
 * @endcode
 *
 * For vectors:
 * @code
 * std::vector<MyStruct> data = {MyStruct{1, 2.0f, "test"}};
 * serde::saveToFile("vector_data.bin", data);
 * auto loaded = serde::loadFromFile<std::vector<MyStruct>>("vector_data.bin");
 * @endcode
 */
namespace serde {

/**
 * @brief Write a POD type to buffer
 * @tparam T POD type that is trivially copyable
 * @param buffer Output buffer to write to
 * @param value Value to write
 */
template<typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
write(std::vector<uint8_t>& buffer, const T& value) {
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&value);
    buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
}

/**
 * @brief Read a POD type from buffer
 * @tparam T POD type that is trivially copyable
 * @param buffer Input buffer to read from
 * @param offset Current offset in buffer (will be updated)
 * @param value Output value to read into
 * @throws std::runtime_error if read would go past buffer end
 */
template<typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
read(const std::vector<uint8_t>& buffer, std::size_t& offset, T& value) {
    if (offset + sizeof(T) > buffer.size()) {
        throw std::runtime_error("serde::read: Attempt to read past buffer end");
    }
    std::memcpy(&value, buffer.data() + offset, sizeof(T));
    offset += sizeof(T);
}

/**
 * @brief Write a std::string to buffer
 * @param buffer Output buffer to write to
 * @param str String to write
 */
inline void write(std::vector<uint8_t>& buffer, const std::string& str) {
    std::size_t size = str.size();
    write(buffer, size);
    buffer.insert(buffer.end(), str.begin(), str.end());
}

/**
 * @brief Read a std::string from buffer
 * @param buffer Input buffer to read from
 * @param offset Current offset in buffer (will be updated)
 * @param str Output string to read into
 * @throws std::runtime_error if read would go past buffer end
 */
inline void read(const std::vector<uint8_t>& buffer, std::size_t& offset, std::string& str) {
    std::size_t size;
    read(buffer, offset, size);
    if (offset + size > buffer.size()) {
        throw std::runtime_error("serde::read: String read would exceed buffer bounds");
    }
    str.assign(reinterpret_cast<const char*>(buffer.data() + offset), size);
    offset += size;
}

/**
 * @brief Write a vector of POD types to buffer
 * @tparam T POD type that is trivially copyable
 * @param buffer Output buffer to write to
 * @param vec Vector to write
 */
template<typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
write(std::vector<uint8_t>& buffer, const std::vector<T>& vec) {
    std::size_t size = vec.size();
    write(buffer, size);
    if (!vec.empty()) {
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(vec.data());
        buffer.insert(buffer.end(), ptr, ptr + sizeof(T) * vec.size());
    }
}

/**
 * @brief Read a vector of POD types from buffer
 * @tparam T POD type that is trivially copyable
 * @param buffer Input buffer to read from
 * @param offset Current offset in buffer (will be updated)
 * @param vec Output vector to read into
 * @throws std::runtime_error if read would go past buffer end
 */
template<typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
read(const std::vector<uint8_t>& buffer, std::size_t& offset, std::vector<T>& vec) {
    std::size_t size;
    read(buffer, offset, size);
    vec.resize(size);
    if (size > 0) {
        std::size_t bytes = size * sizeof(T);
        if (offset + bytes > buffer.size()) {
            throw std::runtime_error("serde::read: Vector read would exceed buffer bounds");
        }
        std::memcpy(vec.data(), buffer.data() + offset, bytes);
        offset += bytes;
    }
}

/**
 * @brief Write a vector of strings to buffer
 * @param buffer Output buffer to write to
 * @param vec Vector of strings to write
 */
inline void write(std::vector<uint8_t>& buffer, const std::vector<std::string>& vec) {
    std::size_t size = vec.size();
    write(buffer, size);
    for (const auto& str : vec) {
        write(buffer, str);
    }
}

/**
 * @brief Read a vector of strings from buffer
 * @param buffer Input buffer to read from
 * @param offset Current offset in buffer (will be updated)
 * @param vec Output vector of strings to read into
 * @throws std::runtime_error if read would go past buffer end
 */
inline void read(const std::vector<uint8_t>& buffer, std::size_t& offset, std::vector<std::string>& vec) {
    std::size_t size;
    read(buffer, offset, size);
    vec.resize(size);
    for (std::size_t i = 0; i < size; ++i) {
        read(buffer, offset, vec[i]);
    }
}

/**
 * @brief Write a vector of user-defined types to buffer
 * @tparam T User-defined type that implements serialize method
 * @param buffer Output buffer to write to
 * @param vec Vector of user-defined objects to write
 */
template<typename T>
typename std::enable_if<!std::is_trivially_copyable<T>::value &&
                       !std::is_same<T, std::string>::value>::type
write(std::vector<uint8_t>& buffer, const std::vector<T>& vec) {
    std::size_t size = vec.size();
    write(buffer, size);
    for (const auto& item : vec) {
        item.serialize(buffer);
    }
}

/**
 * @brief Read a vector of user-defined types from buffer
 * @tparam T User-defined type that implements deserialize method
 * @param buffer Input buffer to read from
 * @param offset Current offset in buffer (will be updated)
 * @param vec Output vector of user-defined objects to read into
 * @throws std::runtime_error if read would go past buffer end
 */
template<typename T>
typename std::enable_if<!std::is_trivially_copyable<T>::value &&
                       !std::is_same<T, std::string>::value>::type
read(const std::vector<uint8_t>& buffer, std::size_t& offset, std::vector<T>& vec) {
    std::size_t size;
    read(buffer, offset, size);
    vec.resize(size);
    for (std::size_t i = 0; i < size; ++i) {
        vec[i].deserialize(buffer, offset);
    }
}

namespace detail {
// Type trait to detect if T is a vector
template<typename T>
struct isVector : std::false_type {};

template<typename T>
struct isVector<std::vector<T>> : std::true_type {};

// Helper functions for serialization
template<typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
serializeImpl(std::vector<uint8_t>& buffer, const T& obj) {
    write(buffer, obj);
}

template<typename T>
typename std::enable_if<!std::is_trivially_copyable<T>::value &&
                       !std::is_same<T, std::string>::value &&
                       !isVector<T>::value>::type
serializeImpl(std::vector<uint8_t>& buffer, const T& obj) {
    obj.serialize(buffer);
}

inline void serializeImpl(std::vector<uint8_t>& buffer, const std::string& str) {
    write(buffer, str);
}

template<typename T>
void serializeImpl(std::vector<uint8_t>& buffer, const std::vector<T>& vec) {
    write(buffer, vec);
}

// Helper functions for deserialization
template<typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
deserializeImpl(const std::vector<uint8_t>& buffer, std::size_t& offset, T& obj) {
    read(buffer, offset, obj);
}

template<typename T>
typename std::enable_if<!std::is_trivially_copyable<T>::value &&
                       !std::is_same<T, std::string>::value &&
                       !isVector<T>::value>::type
deserializeImpl(const std::vector<uint8_t>& buffer, std::size_t& offset, T& obj) {
    obj.deserialize(buffer, offset);
}

inline void deserializeImpl(const std::vector<uint8_t>& buffer, std::size_t& offset, std::string& str) {
    read(buffer, offset, str);
}

template<typename T>
void deserializeImpl(const std::vector<uint8_t>& buffer, std::size_t& offset, std::vector<T>& vec) {
    read(buffer, offset, vec);
}

} // namespace detail

/**
 * @brief Serialize any supported type to byte buffer
 * @tparam T Type to serialize (POD, user-defined struct, or vector)
 * @param obj Object to serialize
 * @return Serialized byte buffer
 */
template<typename T>
std::vector<uint8_t> serialize(const T& obj) {
    std::vector<uint8_t> buffer;
    detail::serializeImpl(buffer, obj);
    return buffer;
}

/**
 * @brief Deserialize any supported type from byte buffer
 * @tparam T Type to deserialize (POD, user-defined struct, or vector)
 * @param buffer Byte buffer containing serialized data
 * @return Deserialized object
 * @throws std::runtime_error if deserialization fails
 */
template<typename T>
T deserialize(const std::vector<uint8_t>& buffer) {
    T obj;
    std::size_t offset = 0;
    detail::deserializeImpl(buffer, offset, obj);
    return obj;
}

/**
 * @brief Save any supported type to binary file
 * @tparam T Type to save (POD, user-defined struct, or vector)
 * @param filename Path to output file
 * @param obj Object to save
 * @throws std::runtime_error if file cannot be opened or written
 */
template<typename T>
void saveToFile(const std::string& filename, const T& obj) {
    auto buffer = serialize(obj);
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("serde::saveToFile: Failed to open file for writing: " + filename);
    }
    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    if (!file) {
        throw std::runtime_error("serde::saveToFile: Failed to write to file: " + filename);
    }
}

/**
 * @brief Load any supported type from binary file
 * @tparam T Type to load (POD, user-defined struct, or vector)
 * @param filename Path to input file
 * @return Loaded object
 * @throws std::runtime_error if file cannot be opened, read, or deserialized
 */
template<typename T>
T loadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("serde::loadFromFile: Failed to open file for reading: " + filename);
    }

    std::size_t size = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    if (!file) {
        throw std::runtime_error("serde::loadFromFile: Failed to read from file: " + filename);
    }

    return deserialize<T>(buffer);
}

/**
 * @brief Get file size in bytes
 * @param filename Path to file
 * @return File size in bytes
 * @throws std::runtime_error if file cannot be accessed
 */
inline std::size_t getFileSize(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("serde::getFileSize: Failed to open file: " + filename);
    }
    return file.tellg();
}

/**
 * @brief Check if file exists and is readable
 * @param filename Path to file
 * @return true if file exists and is readable, false otherwise
 */
inline bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

} // namespace serde

} // namespace utils