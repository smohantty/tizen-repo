#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <type_traits>
#include <cstring>
#include <stdexcept>

namespace serde {

    // ---------- POD Helpers ----------
    template <typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value>::type
    write(std::vector<uint8_t>& buffer, const T& value) {
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&value);
        buffer.insert(buffer.end(), p, p + sizeof(T));
    }

    template <typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value>::type
    read(const std::vector<uint8_t>& buffer, size_t& offset, T& value) {
        if (offset + sizeof(T) > buffer.size())
            throw std::runtime_error("Read past buffer end");
        std::memcpy(&value, buffer.data() + offset, sizeof(T));
        offset += sizeof(T);
    }

    // ---------- std::string ----------
    inline void write(std::vector<uint8_t>& buffer, const std::string& str) {
        size_t sz = str.size();
        write(buffer, sz);
        buffer.insert(buffer.end(), str.begin(), str.end());
    }

    inline void read(const std::vector<uint8_t>& buffer, size_t& offset, std::string& str) {
        size_t sz;
        read(buffer, offset, sz);
        if (offset + sz > buffer.size())
            throw std::runtime_error("Read past buffer end");
        str.assign(reinterpret_cast<const char*>(buffer.data() + offset), sz);
        offset += sz;
    }

    // ---------- std::vector<POD> ----------
    template <typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value>::type
    write(std::vector<uint8_t>& buffer, const std::vector<T>& vec) {
        size_t sz = vec.size();
        serde::write(buffer, sz);
        if (!vec.empty()) {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(vec.data());
            buffer.insert(buffer.end(), p, p + sizeof(T) * vec.size());
        }
    }

    template <typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value>::type
    read(const std::vector<uint8_t>& buffer, size_t& offset, std::vector<T>& vec) {
        size_t sz;
        serde::read(buffer, offset, sz);
        vec.resize(sz);
        if (sz > 0) {
            size_t bytes = sz * sizeof(T);
            if (offset + bytes > buffer.size())
                throw std::runtime_error("Read past buffer end");
            std::memcpy(vec.data(), buffer.data() + offset, bytes);
            offset += bytes;
        }
    }

    // ---------- std::vector<std::string> ----------
    inline void write(std::vector<uint8_t>& buffer, const std::vector<std::string>& vec) {
        size_t sz = vec.size();
        serde::write(buffer, sz);
        for (const auto& s : vec) serde::write(buffer, s);
    }

    inline void read(const std::vector<uint8_t>& buffer, size_t& offset, std::vector<std::string>& vec) {
        size_t sz;
        serde::read(buffer, offset, sz);
        vec.resize(sz);
        for (size_t i = 0; i < sz; ++i) serde::read(buffer, offset, vec[i]);
    }

    // ---------- Custom types with serialize/deserialize methods ----------
    template <typename T>
    typename std::enable_if<std::is_class<T>::value && !std::is_trivially_copyable<T>::value>::type
    write(std::vector<uint8_t>& buffer, const T& obj) {
        obj.serialize(buffer);
    }

    template <typename T>
    typename std::enable_if<std::is_class<T>::value && !std::is_trivially_copyable<T>::value>::type
    read(const std::vector<uint8_t>& buffer, size_t& offset, T& obj) {
        obj.deserialize(buffer, offset);
    }

    // ---------- std::vector<CustomType> ----------
    template <typename T>
    typename std::enable_if<std::is_class<T>::value && !std::is_trivially_copyable<T>::value>::type
    write(std::vector<uint8_t>& buffer, const std::vector<T>& vec) {
        size_t sz = vec.size();
        serde::write(buffer, sz);
        for (const auto& item : vec) serde::write(buffer, item);
    }

    template <typename T>
    typename std::enable_if<std::is_class<T>::value && !std::is_trivially_copyable<T>::value>::type
    read(const std::vector<uint8_t>& buffer, size_t& offset, std::vector<T>& vec) {
        size_t sz;
        serde::read(buffer, offset, sz);
        vec.resize(sz);
        for (size_t i = 0; i < sz; ++i) serde::read(buffer, offset, vec[i]);
    }


    // ---------- Serialize / Deserialize ----------
    template <typename T>
    std::vector<uint8_t> serialize(const T& obj) {
        std::vector<uint8_t> buf;
        obj.serialize(buf);
        return buf;
    }

    template <typename T>
    T deserialize(const std::vector<uint8_t>& buf) {
        T obj;
        size_t offset = 0;
        obj.deserialize(buf, offset);
        return obj;
    }

    // ---------- Save / Load File ----------
    inline void save_file(const std::string& filename, const std::vector<uint8_t>& buf) {
        std::ofstream ofs(filename, std::ios::binary);
        if (!ofs) throw std::runtime_error("Failed to open file for writing");
        ofs.write(reinterpret_cast<const char*>(buf.data()), buf.size());
    }

    inline std::vector<uint8_t> load_file(const std::string& filename) {
        std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
        if (!ifs) throw std::runtime_error("Failed to open file for reading");
        size_t size = ifs.tellg();
        ifs.seekg(0);
        std::vector<uint8_t> buf(size);
        ifs.read(reinterpret_cast<char*>(buf.data()), size);
        return buf;
    }

} // namespace serde
