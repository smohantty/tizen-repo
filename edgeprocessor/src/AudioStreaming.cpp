#include "AudioStreaming.h"
#include "Message.h"
#include "RingBuffer.h"
#include "JsonFormatter.h"
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <utility>
#include <variant>

namespace edgeprocessor {

enum class State {
    Idle,
    Ready,
    Streaming,
    Ending,
    Closed
};

class AudioStreaming::Impl {
public:
    Impl(AudioStreamingConfig config,
         std::shared_ptr<IAudioStreamingListener> listener,
         std::shared_ptr<ITransportAdapter> transportAdapter)
        : mConfig(std::move(config))
        , mListener(std::move(listener))
        , mTransportAdapter(std::move(transportAdapter))
        , mRingBuffer(mConfig.ringBufferSize)
        , mState(State::Idle)
        , mSequenceNumber(0)
        , mRunning(false) {

        // Set up transport callback
        if (mTransportAdapter) {
            mTransportAdapter->setReceiveCallback([this](const std::string& json) {
                handleIncomingMessage(json);
            });
        }
    }

    ~Impl() {
        stop();
    }

    void start() {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mState != State::Idle) {
            return; // Already started or in progress
        }

        // Generate session ID if not provided
        if (mConfig.sessionId.empty()) {
            mConfig.sessionId = JsonFormatter::generateUuid();
        }

        // Enqueue start command
        enqueueMessage(CmdStart{});

        // Start processing thread if not already running
        if (!mRunning) {
            mRunning = true;
            mProcessingThread = std::thread(&Impl::processingLoop, this);
        }
    }

    void continueWithPcm(const void* data, size_t bytes, uint64_t ptsMs) {
        if (!data || bytes == 0) {
            return;
        }

        // Write to ring buffer
        size_t written = mRingBuffer.write(data, bytes);
        if (written == 0) {
            // Buffer is full, drop the data
            return;
        }

        // Enqueue continue command with the written data
        std::vector<uint8_t> pcmData(static_cast<const uint8_t*>(data),
                                    static_cast<const uint8_t*>(data) + written);
        enqueueMessage(CmdContinue{std::move(pcmData), ptsMs});
    }

    void end() {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mState == State::Idle || mState == State::Closed) {
            return; // Not in an active state
        }

        enqueueMessage(CmdEnd{});
    }

    void cancel() {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mState == State::Closed) {
            return; // Already closed
        }

        enqueueMessage(CmdCancel{});
    }

    std::string getSessionId() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mConfig.sessionId;
    }

    bool isActive() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mState != State::Idle && mState != State::Closed;
    }

    std::string getState() const {
        std::lock_guard<std::mutex> lock(mMutex);
        switch (mState) {
            case State::Idle: return "Idle";
            case State::Ready: return "Ready";
            case State::Streaming: return "Streaming";
            case State::Ending: return "Ending";
            case State::Closed: return "Closed";
            default: return "Unknown";
        }
    }

private:
    void enqueueMessage(Message message) {
        std::lock_guard<std::mutex> lock(mMessageMutex);
        mMessageQueue.push(std::move(message));
        mMessageCondition.notify_one();
    }

    void processingLoop() {
        while (mRunning) {
            Message message;

            // Wait for message
            {
                std::unique_lock<std::mutex> lock(mMessageMutex);
                mMessageCondition.wait(lock, [this] {
                    return !mMessageQueue.empty() || !mRunning;
                });

                if (!mRunning) {
                    break;
                }

                if (mMessageQueue.empty()) {
                    continue;
                }

                message = std::move(mMessageQueue.front());
                mMessageQueue.pop();
            }

            // Process message
            std::visit([this](const auto& msg) {
                processMessage(msg);
            }, message);
        }
    }

    void processMessage(const CmdStart&) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mState != State::Idle) {
            return;
        }

        if (!mTransportAdapter || !mTransportAdapter->isConnected()) {
            if (mListener) {
                mListener->onError("Transport not connected");
            }
            mState = State::Closed;
            return;
        }

        // Send start message
        std::string startJson = mJsonFormatter.formatStart(mConfig);
        if (!mTransportAdapter->send(startJson)) {
            if (mListener) {
                mListener->onError("Failed to send start message");
            }
            mState = State::Closed;
            return;
        }

        mState = State::Ready;
        mSequenceNumber = 0;

        if (mListener) {
            mListener->onReady();
        }
    }

    void processMessage(const CmdContinue& cmd) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mState != State::Ready && mState != State::Streaming) {
            return; // Not in a state to accept audio
        }

        if (!mTransportAdapter || !mTransportAdapter->isConnected()) {
            if (mListener) {
                mListener->onError("Transport not connected");
            }
            mState = State::Closed;
            return;
        }

        // Send audio message
        std::string audioJson = mJsonFormatter.formatAudio(
            cmd.pcm, cmd.ptsMs, ++mSequenceNumber, false);

        if (!mTransportAdapter->send(audioJson)) {
            if (mListener) {
                mListener->onError("Failed to send audio data");
            }
            mState = State::Closed;
            return;
        }

        if (mState == State::Ready) {
            mState = State::Streaming;
        }
    }

    void processMessage(const CmdEnd&) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mState != State::Ready && mState != State::Streaming) {
            return;
        }

        if (!mTransportAdapter || !mTransportAdapter->isConnected()) {
            if (mListener) {
                mListener->onError("Transport not connected");
            }
            mState = State::Closed;
            return;
        }

        // Send end message
        std::string endJson = mJsonFormatter.formatEnd(++mSequenceNumber);
        if (!mTransportAdapter->send(endJson)) {
            if (mListener) {
                mListener->onError("Failed to send end message");
            }
            mState = State::Closed;
            return;
        }

        mState = State::Ending;
    }

    void processMessage(const CmdCancel&) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mState == State::Closed) {
            return;
        }

        mState = State::Closed;

        if (mListener) {
            mListener->onStatus("Session cancelled");
            mListener->onClosed();
        }
    }

    void processMessage(const EvPartial& ev) {
        if (mListener) {
            mListener->onPartialResult(ev.text, ev.stability);
        }
    }

    void processMessage(const EvFinal& ev) {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mListener) {
            mListener->onFinalResult(ev.text, ev.confidence);
        }

        if (mState == State::Ending) {
            mState = State::Closed;
            if (mListener) {
                mListener->onClosed();
            }
        }
    }

    void processMessage(const EvLatency& ev) {
        if (mListener) {
            mListener->onLatency(ev.upstreamMs, ev.e2eMs);
        }
    }

    void processMessage(const EvStatus& ev) {
        if (mListener) {
            mListener->onStatus(ev.message);
        }
    }

    void processMessage(const EvError& ev) {
        std::lock_guard<std::mutex> lock(mMutex);

        mState = State::Closed;

        if (mListener) {
            mListener->onError(ev.what);
            mListener->onClosed();
        }
    }

    void processMessage(const EvClosed&) {
        std::lock_guard<std::mutex> lock(mMutex);

        mState = State::Closed;

        if (mListener) {
            mListener->onClosed();
        }
    }

    void handleIncomingMessage(const std::string& json) {
        try {
            // Simple message type detection
            if (json.find("\"type\":\"partial\"") != std::string::npos) {
                enqueueMessage(mJsonFormatter.parsePartial(json));
            } else if (json.find("\"type\":\"final\"") != std::string::npos) {
                enqueueMessage(mJsonFormatter.parseFinal(json));
            } else if (json.find("\"type\":\"latency\"") != std::string::npos) {
                enqueueMessage(mJsonFormatter.parseLatency(json));
            } else if (json.find("\"type\":\"status\"") != std::string::npos) {
                enqueueMessage(mJsonFormatter.parseStatus(json));
            } else if (json.find("\"type\":\"error\"") != std::string::npos) {
                enqueueMessage(mJsonFormatter.parseError(json));
            } else {
                // Unknown message type, treat as status
                enqueueMessage(EvStatus{json});
            }
        } catch (const std::exception& e) {
            enqueueMessage(EvError{std::string("JSON parse error: ") + e.what()});
        }
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mMessageMutex);
            mRunning = false;
            mMessageCondition.notify_all();
        }

        if (mProcessingThread.joinable()) {
            mProcessingThread.join();
        }
    }

    AudioStreamingConfig mConfig;
    std::shared_ptr<IAudioStreamingListener> mListener;
    std::shared_ptr<ITransportAdapter> mTransportAdapter;
    RingBuffer mRingBuffer;
    JsonFormatter mJsonFormatter;

    State mState;
    uint32_t mSequenceNumber;
    std::atomic<bool> mRunning;

    std::queue<Message> mMessageQueue;
    mutable std::mutex mMutex;
    mutable std::mutex mMessageMutex;
    std::condition_variable mMessageCondition;
    std::thread mProcessingThread;
};

// AudioStreaming implementation
AudioStreaming::AudioStreaming(AudioStreamingConfig config,
                               std::shared_ptr<IAudioStreamingListener> listener,
                               std::shared_ptr<ITransportAdapter> transportAdapter)
    : mPimpl(std::make_unique<Impl>(std::move(config),
                                   std::move(listener),
                                   std::move(transportAdapter))) {
}

AudioStreaming::~AudioStreaming() = default;

void AudioStreaming::start() {
    mPimpl->start();
}

void AudioStreaming::continueWithPcm(const void* data, size_t bytes, uint64_t ptsMs) {
    mPimpl->continueWithPcm(data, bytes, ptsMs);
}

void AudioStreaming::end() {
    mPimpl->end();
}

void AudioStreaming::cancel() {
    mPimpl->cancel();
}

std::string AudioStreaming::getSessionId() const {
    return mPimpl->getSessionId();
}

bool AudioStreaming::isActive() const {
    return mPimpl->isActive();
}

std::string AudioStreaming::getState() const {
    return mPimpl->getState();
}

} // namespace edgeprocessor
