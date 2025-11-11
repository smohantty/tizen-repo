#ifndef TTS_ENGINE_H
#define TTS_ENGINE_H

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <map>

namespace utils {

enum class AudioFormat {
    Binary,
    Base64
};

enum class TtsError {
    None,
    NetworkError,
    InvalidResponse,
    DecodingError,
    RequestCancelled
};

using AudioChunkCallback = std::function<void(const std::vector<uint8_t>& audioData, TtsError error)>;

struct TtsConfig {
    std::string apiUrl;
    AudioFormat format = AudioFormat::Binary;
    std::map<std::string, std::string> headers;
    long timeoutMs = 30000;
    size_t maxChunkSize = 8192;
};

class TtsEngine {
public:
    TtsEngine();
    explicit TtsEngine(const TtsConfig& config);
    ~TtsEngine();

    // Configure the TTS engine
    void setConfig(const TtsConfig& config);
    TtsConfig getConfig() const;

    // Synchronous text-to-speech conversion with streaming callback
    TtsError synthesize(const std::string& text,
                       AudioChunkCallback callback,
                       const std::map<std::string, std::string>& additionalParams = {});

    // Cancel ongoing synthesis
    void cancel();

    // Check if synthesis is in progress
    bool isSynthesizing() const;

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace utils

#endif // TTS_ENGINE_H

