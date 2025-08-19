#pragma once
#include <memory>
#include <vector>
#include <string>

namespace wakeword {

class WakeupWordDetector {
public:
    // Constructor with model file path and optional parameters
    explicit WakeupWordDetector(const std::string& modelFilePath);
    ~WakeupWordDetector();

    // Buffers input until at least a minimum chunk size, then processes
    bool processAudioBuffer(const std::vector<short>& buffer);
    bool isWakeupWordDetected() const;

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace wakeword
