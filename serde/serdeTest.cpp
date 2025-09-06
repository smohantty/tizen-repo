#include <iostream>
#include <string>
#include <vector>
#include "serde.h"

// POD + string + vector struct
struct MyData {
    std::string name;
    std::vector<float> values;
    std::vector<std::string> labels;
    float score;

    void serialize(std::vector<uint8_t>& buf) const {
        serde::write(buf, name);
        serde::write(buf, values);
        serde::write(buf, labels);
        serde::write(buf, score);
    }

    void deserialize(const std::vector<uint8_t>& buf, size_t& offset) {
        serde::read(buf, offset, name);
        serde::read(buf, offset, values);
        serde::read(buf, offset, labels);
        serde::read(buf, offset, score);
    }
};

// Container struct
struct Profiles {
    std::vector<MyData> users;

    void serialize(std::vector<uint8_t>& buf) const {
        size_t sz = users.size();
        serde::write(buf, sz);
        for (const auto& u : users) u.serialize(buf);
    }

    void deserialize(const std::vector<uint8_t>& buf, size_t& offset) {
        size_t sz;
        serde::read(buf, offset, sz);
        users.resize(sz);
        for (size_t i = 0; i < sz; ++i) users[i].deserialize(buf, offset);
    }
};

int main() {
    Profiles profiles;

    // Test with English labels
    profiles.users.push_back({"Alice", {1.2,2.43,3.6}, {"one","two","three"}, 9.5f});
    profiles.users.push_back({"Bob", {4.5,5.6,6.7}, {"four","five","six"}, 8.2f});

    // Test with Korean labels
    profiles.users.push_back({"김철수", {7.8,8.9,9.0}, {"하나","둘","셋"}, 7.8f});
    profiles.users.push_back({"이영희", {10.1,11.2,12.3}, {"넷","다섯","여섯"}, 8.9f});

    // Test with mixed English and Korean labels
    profiles.users.push_back({"John Kim", {13.4,14.5,15.6}, {"seven","일곱","eight"}, 9.1f});

    // Serialize
    auto buf = serde::serialize(profiles);
    serde::save_file("profiles.bin", buf);

    // Load and deserialize
    auto loaded_buf = serde::load_file("profiles.bin");
    Profiles copy = serde::deserialize<Profiles>(loaded_buf);

    // Print results
    std::cout << "=== Serde Test Results (English & Korean Labels) ===\n\n";
    for (auto& u : copy.users) {
        std::cout << "User: " << u.name << ", Score: " << u.score << "\nValues: ";
        for (auto v : u.values) std::cout << v << " ";
        std::cout << "\nLabels: ";
        for (auto& s : u.labels) std::cout << s << " ";
        std::cout << "\n\n";
    }

    // Verify data integrity
    std::cout << "=== Data Integrity Check ===\n";
    std::cout << "Original users: " << profiles.users.size() << "\n";
    std::cout << "Deserialized users: " << copy.users.size() << "\n";

    bool allMatch = true;
    for (size_t i = 0; i < profiles.users.size() && i < copy.users.size(); ++i) {
        if (profiles.users[i].name != copy.users[i].name ||
            profiles.users[i].score != copy.users[i].score ||
            profiles.users[i].values != copy.users[i].values ||
            profiles.users[i].labels != copy.users[i].labels) {
            allMatch = false;
            break;
        }
    }

    std::cout << "All data matches: " << (allMatch ? "PASS" : "FAIL") << "\n";
}
