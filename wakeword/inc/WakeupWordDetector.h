#pragma once
#include <memory>
#include <vector>

namespace wakeword {

class WakeupWordDetector {
public:
    WakeupWordDetector();
    ~WakeupWordDetector();

    // Buffers input until at least a minimum chunk size, then processes
    bool processAudioBuffer(const std::vector<short>& buffer);
    bool isWakeupWordDetected() const;

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace wakeword
