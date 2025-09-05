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
    profiles.users.push_back({"Alice", {1.2,2.43,3.6}, {"one","two"}, 9.5f});
    profiles.users.push_back({"Bob", {4.5,5.6,6.7}, {"four","five"}, 8.2f});

    // Serialize
    auto buf = serde::serialize(profiles);
    serde::save_file("profiles.bin", buf);

    // Load and deserialize
    auto loaded_buf = serde::load_file("profiles.bin");
    Profiles copy = serde::deserialize<Profiles>(loaded_buf);

    // Print results
    for (auto& u : copy.users) {
        std::cout << "User: " << u.name << ", Score: " << u.score << "\nValues: ";
        for (auto v : u.values) std::cout << v << " ";
        std::cout << "\nLabels: ";
        for (auto& s : u.labels) std::cout << s << " ";
        std::cout << "\n\n";
    }
}
