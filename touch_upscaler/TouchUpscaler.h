#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>

using namespace std::chrono;

namespace vtd {

struct TouchSample {
    float x{0.0f};
    float y{0.0f};
    bool valid{false};
};

enum class Backend { SingleTouchDevice, Mock };

struct Config {
    Backend backend{Backend::Mock};
    bool enableRecording{false};

    int screenWidth = 1920;
    int screenHeight = 1080;

    double outputHz = 130.0;

    size_t historySize = 6;

    std::string deviceName = "IR Touch";

    static Config getDefault();
};

class TouchUpscaler {
   public:
    explicit TouchUpscaler(const Config& cfg);
    ~TouchUpscaler();

    void start();
    void stop();
    void push(const TouchSample& sample);

   private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace vtd
