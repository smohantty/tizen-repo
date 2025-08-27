# Voice Activity Detection (VAD) Library

A C++17 Voice Activity Detection system that detects the start and end of speech by continuously processing a stream of audio buffers.

## Features

- **Simple API**: Easy-to-use interface for voice activity detection
- **Real-time processing**: Processes audio buffers continuously
- **Configurable parameters**: Adjustable thresholds and timing parameters
- **Event callbacks**: Get notified when speech starts and ends
- **Static library**: Builds as a static library for easy integration
- **Meson build system**: Supports both standalone and subproject usage

## Building

### Standalone Build

```bash
cd vad
meson setup build
meson compile -C build
```

### As a Subproject

Add this project as a subproject in your main project and use the `vad_dep` dependency.

## API Usage

### Basic Usage

```cpp
#include "VoiceActivityDetector.h"

// Create detector with default parameters (16kHz, 100ms frames)
vad::VoiceActivityDetector detector;

// Process audio buffer (16-bit signed integers)
std::vector<short> audioBuffer = getAudioData();
bool stateChanged = detector.processAudioBuffer(audioBuffer);

// Check if speech is currently active
if (detector.isSpeechActive()) {
    std::cout << "Speech detected!" << std::endl;
}
```

### Advanced Usage with Callbacks

```cpp
#include "VoiceActivityDetector.h"

// Create detector with custom parameters
vad::VoiceActivityDetector detector(16000, 1600); // 16kHz, 100ms frames

// Set up event callback
detector.setSpeechEventCallback([](bool isSpeechActive, uint64_t timestamp) {
    if (isSpeechActive) {
        std::cout << "Speech started at " << timestamp << "ms" << std::endl;
    } else {
        std::cout << "Speech ended at " << timestamp << "ms" << std::endl;
    }
});

// Configure detection parameters
detector.setEnergyThreshold(5000.0);        // Energy threshold
detector.setMinSpeechDuration(150);         // Minimum speech duration (ms)
detector.setMinSilenceDuration(300);        // Minimum silence duration (ms)

// Process audio continuously
while (hasMoreAudio()) {
    std::vector<short> buffer = getNextAudioBuffer();
    detector.processAudioBuffer(buffer);
}
```

## API Reference

### Constructor

```cpp
VoiceActivityDetector(int sampleRate = 16000, size_t frameSize = 1600)
```

- `sampleRate`: Audio sample rate in Hz
- `frameSize`: Number of samples per processing frame

### Core Methods

```cpp
bool processAudioBuffer(const std::vector<short>& buffer)
```
Process a buffer of audio samples. Returns true if voice activity state changed.

```cpp
bool isSpeechActive() const
```
Check if speech is currently detected.

```cpp
uint64_t getLastStateChangeTimestamp() const
```
Get timestamp of last speech state change in milliseconds.

### Configuration Methods

```cpp
void setEnergyThreshold(double threshold)
```
Set energy threshold for voice detection (higher = less sensitive).

```cpp
void setMinSpeechDuration(uint64_t durationMs)
```
Set minimum speech duration to trigger speech start event.

```cpp
void setMinSilenceDuration(uint64_t durationMs)
```
Set minimum silence duration to trigger speech end event.

```cpp
void setSpeechEventCallback(const SpeechEventCallback& callback)
```
Set callback function for speech start/end events.

```cpp
void reset()
```
Reset detector state.

## Example

Run the included example to see the VAD system in action:

```bash
./build/vad_example
```

The example demonstrates:
- Processing different types of audio (silence, speech simulation)
- Real-time processing with small buffer chunks
- Event callbacks for speech start/end detection
- Configuration of detection parameters

## Algorithm

The VAD system uses:

1. **Energy-based detection**: RMS energy calculation for voice activity
2. **Temporal smoothing**: Majority voting over recent frames to reduce false positives
3. **Duration thresholds**: Configurable minimum durations for speech and silence
4. **State machine**: Tracks speech/silence states with hysteresis

## Integration

### Using as a Subproject

1. Add to your `subprojects` directory
2. In your `meson.build`:

```meson
vad_dep = dependency('vad', fallback : ['vad', 'vad_dep'])

executable('your_app', 'main.cpp', dependencies: vad_dep)
```

### Namespace

All classes are in the `vad` namespace:

```cpp
vad::VoiceActivityDetector detector;
```

## Requirements

- C++17 compatible compiler
- Meson build system
- Audio data in 16-bit signed integer format

## License

This project follows the same license as the parent repository.
