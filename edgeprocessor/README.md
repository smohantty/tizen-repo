# AudioStreaming Module for Edge ASR

A C++17 library for streaming audio to Edge ASR (Automatic Speech Recognition) services. This module provides a simple API to handle audio streaming, buffering, session management, and JSON message formatting for communication with Edge ASR services.

## Features

- **Simple API**: Three main methods (`start()`, `continueWithPcm()`, `end()`)
- **Thread-safe**: All public methods are thread-safe and non-blocking
- **Configurable**: Support for different audio formats and buffer sizes
- **State Management**: Robust state machine for session lifecycle
- **JSON Handling**: Built-in JSON formatting and parsing for ASR messages
- **Base64 Encoding**: Audio data encoding for transport
- **Error Handling**: Comprehensive error handling with callback notifications
- **Transport Agnostic**: Pluggable transport adapter interface

## Architecture

The module consists of several key components:

- **AudioStreaming**: Main public API class
- **RingBuffer**: Thread-safe circular buffer for audio data
- **JsonFormatter**: JSON encoding/decoding utilities
- **Message System**: Internal message passing using `std::variant`
- **State Machine**: Session lifecycle management
- **Transport Adapter**: Interface for external transport libraries

## Quick Start

### Basic Usage

```cpp
#include "AudioStreaming.h"

// Create configuration
AudioStreamingConfig config;
config.sampleRateHz = 16000;
config.bitsPerSample = 16;
config.channels = 1;

// Create listener
class MyListener : public IAudioStreamingListener {
    void onReady() override { /* Session ready */ }
    void onPartialResult(const std::string& text, float stability) override { /* Partial result */ }
    void onFinalResult(const std::string& text, float confidence) override { /* Final result */ }
    void onError(const std::string& error) override { /* Handle error */ }
    void onClosed() override { /* Session closed */ }
    // ... other callbacks
};

// Create transport adapter (implement ITransportAdapter)
auto transport = std::make_shared<MyTransportAdapter>();

// Create AudioStreaming instance
auto listener = std::make_shared<MyListener>();
AudioStreaming streaming(config, listener, transport);

// Start streaming
streaming.start();

// Send audio data
uint8_t audioData[] = { /* PCM audio data */ };
streaming.continueWithPcm(audioData, sizeof(audioData), ptsMs);

// End streaming
streaming.end();
```

### Transport Adapter Implementation

```cpp
class MyTransportAdapter : public ITransportAdapter {
public:
    bool send(const std::string& jsonMessage) override {
        // Send JSON message to your ASR service
        return myTransportLibrary.send(jsonMessage);
    }

    void setReceiveCallback(std::function<void(const std::string&)> callback) override {
        mCallback = callback;
        // Set up your transport to call this callback when messages arrive
    }

    bool isConnected() const override {
        return myTransportLibrary.isConnected();
    }

private:
    std::function<void(const std::string&)> mCallback;
};
```

## Building

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Meson build system
- Threading library (pthread on Unix, Windows threads on Windows)

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd edgeprocessor

# Configure build
meson setup builddir

# Build
meson compile -C builddir

# Run tests (optional)
meson test -C builddir

# Run example (optional)
./builddir/audio_streaming_example
```

### Build Options

- `examples`: Build example application (default: true)
- `tests`: Build and run tests (default: true)

```bash
# Configure with options
meson setup builddir -Dexamples=false -Dtests=true
```

### Using as a Subproject

Add to your project's `meson.build`:

```meson
# Add as subproject
edgeprocessor_dep = dependency('edgeprocessor', fallback: ['edgeprocessor', 'edgeprocessor_dep'])

# Use in your target
executable('myapp', 'src/main.cpp', dependencies: [edgeprocessor_dep])
```

## API Reference

### AudioStreamingConfig

Configuration struct for audio streaming parameters:

```cpp
struct AudioStreamingConfig {
    uint32_t sampleRateHz = 16000;        // Audio sample rate
    uint8_t bitsPerSample = 16;           // Bits per sample
    uint8_t channels = 1;                 // Number of channels
    std::string sessionId;                // Session ID (auto-generated if empty)
    size_t chunkDurationMs = 20;          // Audio chunk duration
    size_t ringBufferSize = 1024 * 1024;  // Ring buffer size in bytes
};
```

### IAudioStreamingListener

Interface for receiving streaming events:

```cpp
class IAudioStreamingListener {
    virtual void onReady() = 0;                                    // Session ready
    virtual void onPartialResult(const std::string& text, float stability) = 0;
    virtual void onFinalResult(const std::string& text, float confidence) = 0;
    virtual void onLatency(uint32_t upstreamMs, uint32_t e2eMs) = 0;
    virtual void onStatus(const std::string& message) = 0;
    virtual void onError(const std::string& error) = 0;
    virtual void onClosed() = 0;                                   // Session closed
};
```

### AudioStreaming

Main streaming class:

```cpp
class AudioStreaming {
public:
    // Constructor
    AudioStreaming(AudioStreamingConfig config,
                   std::shared_ptr<IAudioStreamingListener> listener,
                   std::shared_ptr<ITransportAdapter> transportAdapter);

    // Core API
    void start();                                    // Start streaming session
    void continueWithPcm(const void* data, size_t bytes, uint64_t ptsMs);
    void end();                                      // End streaming session
    void cancel();                                   // Cancel session

    // State queries
    std::string getSessionId() const;
    bool isActive() const;
    std::string getState() const;
};
```

## JSON Message Format

### Outbound Messages (Client → ASR Service)

**Start Message:**
```json
{
  "type": "start",
  "session_id": "uuid-1234",
  "format": {
    "sample_rate_hz": 16000,
    "bits_per_sample": 16,
    "channels": 1
  },
  "options": {
    "partial_results": true,
    "compression": "pcm16"
  }
}
```

**Audio Message:**
```json
{
  "type": "audio",
  "seq": 42,
  "pts_ms": 12340,
  "last": false,
  "payload": "base64-encoded-audio"
}
```

**End Message:**
```json
{
  "type": "end",
  "seq": 99,
  "last": true
}
```

### Inbound Messages (ASR Service → Client)

**Partial Result:**
```json
{
  "type": "partial",
  "text": "hello wor",
  "stability": 0.85
}
```

**Final Result:**
```json
{
  "type": "final",
  "text": "hello world",
  "confidence": 0.94
}
```

**Latency Report:**
```json
{
  "type": "latency",
  "upstream_ms": 42,
  "e2e_ms": 120
}
```

## State Machine

The module implements a state machine with the following states:

- **Idle**: Initial state, before `start()` is called
- **Ready**: After sending start message, ready to receive audio
- **Streaming**: Actively streaming audio data
- **Ending**: `end()` called, waiting for final result
- **Closed**: Session finished or error occurred

### State Transitions

- `Idle` → `start()` → `Ready`
- `Ready/Streaming` → `continueWithPcm()` → `Streaming`
- `Streaming` → `end()` → `Ending`
- `Ending` → receive final result → `Closed`
- Any state → error → `Closed`

## Error Handling

The module provides comprehensive error handling:

- **Transport Errors**: Connection failures, send/receive errors
- **JSON Parsing Errors**: Invalid message format
- **State Errors**: Invalid operations for current state
- **Buffer Errors**: Ring buffer overflow/underflow

All errors are reported via the `onError()` callback with descriptive messages.

## Threading Model

- **Thread-safe API**: All public methods can be called from any thread
- **Non-blocking**: API calls return immediately, no blocking operations
- **Single Producer**: One thread processes messages internally
- **Callback Thread**: Listener callbacks are invoked from the internal processing thread

## Performance Considerations

- **Efficient Buffering**: Ring buffer with minimal memory copies
- **Configurable Chunk Size**: Optimize for your audio format
- **Base64 Encoding**: Efficient encoding/decoding implementation
- **Memory Management**: RAII principles, smart pointers, move semantics

## Examples

See the `example/` directory for complete usage examples:

- `audio_streaming_example.cpp`: Basic streaming demonstration
- Mock transport adapter implementation
- Error handling examples

## Testing

Run the test suite:

```bash
meson test -C builddir
```

Tests cover:
- RingBuffer operations and thread safety
- JSON formatting and parsing
- State machine transitions
- Error handling scenarios

## Contributing

1. Follow the existing code style (C++17, Pimpl idiom, 'm' prefix for member variables)
2. Add tests for new functionality
3. Update documentation as needed
4. Ensure all tests pass before submitting

## License

[Add your license information here]

## Support

For issues and questions:
- Create an issue on the project repository
- Check the examples for usage patterns
- Review the test suite for implementation details
