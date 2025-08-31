# Plan: C++17 AudioStreaming Module for Edge ASR

This document describes the architecture, API, message formats, and implementation plan for building a C++17 **AudioStreaming** module. The module provides a simple API (`start`, `continue`, `end`) to stream audio to an Edge ASR service via an external transport library (that exchanges JSON messages with the edge server).

---

## 1. Goals

* Abstract away audio streaming and transport complexity.
* Provide simple API: `start()`, `continueWithPcm()`, `end()`.
* Handle buffering, sequencing, session lifecycle, and JSON message formatting.
* Parse ASR results (`partial`, `final`, `latency`, `status`) from JSON messages.
* Deliver results via listener callbacks.

---

## 2. Public API

```cpp
struct AudioStreamingConfig {
    uint32_t sample_rate_hz = 16000;
    uint8_t bits_per_sample = 16;
    uint8_t channels = 1;
    std::string session_id;       // generated per stream
    size_t chunk_duration_ms = 20;
};

class IAudioStreamingListener {
public:
    virtual ~IAudioStreamingListener() = default;
    virtual void onReady() = 0;
    virtual void onPartialResult(const std::string& text, float stability) = 0;
    virtual void onFinalResult(const std::string& text, float confidence) = 0;
    virtual void onLatency(uint32_t upstream_ms, uint32_t e2e_ms) = 0;
    virtual void onStatus(const std::string& message) = 0;
    virtual void onError(const std::string& error) = 0;
    virtual void onClosed() = 0;
};

class AudioStreaming {
public:
    explicit AudioStreaming(AudioStreamingConfig cfg,
                            std::shared_ptr<IAudioStreamingListener> listener);

    void start();
    void continueWithPcm(const void* data, size_t bytes, uint64_t pts_ms);
    void end();
    void cancel();
};
```

---

## 3. Internal Architecture

### Components

* **Public Facade**: API entrypoints; enqueues commands.
* **Core State Machine**: manages session lifecycle.
* **Message Bus**: `std::variant` queue of commands/events.
* **JSON Formatter**: converts commands → JSON outbound messages.
* **JSON Parser**: interprets inbound JSON messages → events.
* **External Transport Adapter**: provided by existing library (send/receive JSON strings).

### Concurrency Model

* API calls enqueue commands into a thread-safe queue.
* Core thread pops messages and processes with `std::visit`.
* Listener callbacks invoked from Core thread.

---

## 4. State Machine

### States

* **Idle**: before `start()`.
* **Ready**: after sending `start` message.
* **Streaming**: sending audio chunks (`audio` messages).
* **Ending**: `end()` called; waiting for final.
* **Closed**: stream finished or error.

### Transitions

* `Idle` → `start()` → send `{type:start}` → `Ready`.
* `Ready/Streaming` → `continueWithPcm()` → send `{type:audio}`.
* `Streaming` → `end()` → send `{type:end}` → `Ending`.
* `Ending` → receive `{type:final}` → `Closed`.
* Any → receive error → `Closed`.

---

## 5. JSON Message Formats

### Outbound (client → edge)

**Start**

```json
{
  "type": "start",
  "session_id": "uuid-1234",
  "format": {"sample_rate_hz":16000,"bits_per_sample":16,"channels":1},
  "options": {"partial_results": true, "compression":"pcm16"}
}
```

**Audio**

```json
{
  "type": "audio",
  "seq": 42,
  "pts_ms": 12340,
  "last": false,
  "payload": "base64-encoded-audio"
}
```

**End**

```json
{
  "type": "end",
  "seq": 99,
  "last": true
}
```

### Inbound (edge → client)

**Partial Result**

```json
{"type":"partial", "text":"hello wor", "stability":0.85}
```

**Final Result**

```json
{"type":"final", "text":"hello world", "confidence":0.94}
```

**Latency Report**

```json
{"type":"latency", "upstream_ms":42, "e2e_ms":120}
```

**Status**

```json
{"type":"status", "message":"stream closed normally"}
```

---

## 6. Internal Messages (`std::variant`)

```cpp
struct CmdStart { };
struct CmdContinue { std::vector<uint8_t> pcm; uint64_t pts_ms; };
struct CmdEnd { };
struct CmdCancel { };

struct EvPartial { std::string text; float stability; };
struct EvFinal { std::string text; float confidence; };
struct EvLatency { uint32_t upstream_ms; uint32_t e2e_ms; };
struct EvStatus { std::string message; };
struct EvError { std::string what; };
struct EvClosed { };

using Message = std::variant<CmdStart, CmdContinue, CmdEnd, CmdCancel,
                             EvPartial, EvFinal, EvLatency, EvStatus, EvError, EvClosed>;
```

---

## 7. Processing Loop (Core Thread)

* Pop a `Message`.
* `std::visit` dispatches:

  * **Cmd**\* → Format JSON, call external transport `send()`.
  * **Ev**\* → Invoke listener callback.

---

## 8. Buffering & Framing

* Incoming PCM buffered in a ring buffer.
* Frames split into chunks of `chunk_duration_ms`.
* Each chunk base64-encoded and sent in an `audio` JSON.
* Sequence numbers incremented monotonically.

---

## 9. Error Handling

* On JSON parse failure → `onError()`.
* On transport error → `onError()` + transition to `Closed`.
* If user calls `continueWithPcm` after `end()` → ignore.

---

## 10. Deliverables

* `audio_streaming.hpp/cpp`: main API.
* `message.hpp`: command/event variant definitions.
* `ring_buffer.hpp/cpp`: PCM ring buffer.
* `json_formatter.hpp/cpp`: JSON encode/decode utilities.
* Unit tests:

  * state transitions,
  * JSON formatting,
  * parsing inbound messages.

