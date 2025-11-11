#ifndef TTS_ENGINE_H
#define TTS_ENGINE_H

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <map>
#include <future>

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

// Result structure for synchronous API
struct SynthesisResult {
    std::vector<uint8_t> audioData;
    TtsError error;

    // Convenience methods
    bool isSuccess() const { return error == TtsError::None; }
    bool isEmpty() const { return audioData.empty(); }
};

class TtsEngine {
public:
    TtsEngine();
    explicit TtsEngine(const TtsConfig& config);
    ~TtsEngine();

    // Configure the TTS engine
    void setConfig(const TtsConfig& config);
    TtsConfig getConfig() const;

    // ============================================================================
    // STREAMING CALLBACK API (Asynchronous with real-time chunks)
    // ============================================================================
    // Best for: Real-time audio playback, memory efficiency, progressive processing
    //
    // This version calls the callback for each chunk as it arrives from the server.
    // The callback receives:
    // - Non-empty audioData: A chunk of audio data
    // - Empty audioData: Signals completion (check error for success/failure)
    //
    // Example:
    //   engine.synthesize("Hello", [](const auto& data, TtsError err) {
    //       if (err != TtsError::None) {
    //           std::cerr << "Error!\n";
    //       } else if (!data.empty()) {
    //           audioPlayer->play(data);  // Play immediately
    //       } else {
    //           std::cout << "Done!\n";
    //       }
    //   });
    TtsError synthesize(const std::string& text,
                       AudioChunkCallback callback,
                       const std::map<std::string, std::string>& additionalParams = {});

    // ============================================================================
    // SYNCHRONOUS BLOCKING API (Returns all audio at once)
    // ============================================================================
    // Best for: Simple use cases, batch processing, when you need all audio before proceeding
    //
    // This version blocks until all audio is received and returns it as a single buffer.
    // Returns SynthesisResult containing all audio data and error status.
    //
    // Example:
    //   auto result = engine.synthesizeSync("Hello");
    //   if (result.isSuccess()) {
    //       audioPlayer->play(result.audioData);
    //   }
    SynthesisResult synthesizeSync(const std::string& text,
                                   const std::map<std::string, std::string>& additionalParams = {});

    // ============================================================================
    // ASYNCHRONOUS NON-BLOCKING API (Future-based)
    // ============================================================================
    // Best for: Parallel processing, UI responsiveness, when you want to start
    //          synthesis and do other work while waiting
    //
    // This version returns immediately with a future. You can wait() or get() the result
    // when needed, or check if it's ready with wait_for().
    //
    // Example:
    //   auto future = engine.synthesizeAsync("Hello");
    //   // Do other work...
    //   auto result = future.get();  // Blocks until ready
    //   if (result.isSuccess()) {
    //       audioPlayer->play(result.audioData);
    //   }
    std::future<SynthesisResult> synthesizeAsync(const std::string& text,
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

