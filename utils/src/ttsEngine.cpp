#include "ttsEngine.h"
#include "Base64.h"
#include <cpr/cpr.h>
#include <iostream>
#include <atomic>
#include <mutex>

namespace utils {

class TtsEngine::Impl {
public:
    Impl() : mCancelled(false), mSynthesizing(false) {}

    explicit Impl(const TtsConfig& config)
        : mConfig(config), mCancelled(false), mSynthesizing(false) {}

    void setConfig(const TtsConfig& config) {
        std::lock_guard<std::mutex> lock(mMutex);
        mConfig = config;
    }

    TtsConfig getConfig() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mConfig;
    }

    TtsError synthesize(const std::string& text,
                       AudioChunkCallback callback,
                       const std::map<std::string, std::string>& additionalParams) {
        if (mSynthesizing.load()) {
            return TtsError::RequestCancelled;
        }

        mSynthesizing.store(true);
        mCancelled.store(false);

        TtsError result = TtsError::None;

        try {
            // Build request body
            std::string requestBody = buildRequestBody(text, additionalParams);

            // Build headers
            cpr::Header headers;
            {
                std::lock_guard<std::mutex> lock(mMutex);
                for (const auto& [key, value] : mConfig.headers) {
                    headers[key] = value;
                }
            }

            // Add content-type if not specified
            if (headers.find("Content-Type") == headers.end()) {
                headers["Content-Type"] = "application/json";
            }

            // Buffer for accumulating data (for base64 decoding)
            std::string base64Buffer;
            std::vector<uint8_t> binaryBuffer;

            // Define write callback for streaming response
            auto writeCallback = [&](const std::string& data, intptr_t userdata) -> bool {
                if (mCancelled.load()) {
                    return false; // Stop receiving data
                }

                AudioFormat format;
                {
                    std::lock_guard<std::mutex> lock(mMutex);
                    format = mConfig.format;
                }

                if (format == AudioFormat::Binary) {
                    // Direct binary data
                    std::vector<uint8_t> chunk(data.begin(), data.end());
                    callback(chunk, TtsError::None);
                } else {
                    // Base64 encoded data - accumulate and decode
                    base64Buffer += data;

                    // Decode complete base64 chunks (must be multiple of 4)
                    size_t completeChunks = (base64Buffer.size() / 4) * 4;
                    if (completeChunks > 0) {
                        std::string toDecode = base64Buffer.substr(0, completeChunks);
                        base64Buffer = base64Buffer.substr(completeChunks);

                        try {
                            std::vector<uint8_t> decoded = Base64::decode(toDecode);
                            if (!decoded.empty()) {
                                callback(decoded, TtsError::None);
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "Base64 decode error: " << e.what() << std::endl;
                            result = TtsError::DecodingError;
                            return false;
                        }
                    }
                }

                return true; // Continue receiving data
            };

            // Make POST request with streaming
            cpr::Session session;
            session.SetUrl(cpr::Url{getApiUrl()});
            session.SetHeader(headers);
            session.SetBody(cpr::Body{requestBody});
            session.SetTimeout(cpr::Timeout{getTimeoutMs()});

            // Set write callback for streaming
            session.SetWriteCallback(cpr::WriteCallback{
                [&writeCallback](std::string data, intptr_t userdata) -> bool {
                    return writeCallback(data, userdata);
                },
                0
            });

            auto response = session.Post();

            // Handle remaining base64 data if any
            if (!base64Buffer.empty() && mConfig.format == AudioFormat::Base64) {
                try {
                    std::vector<uint8_t> decoded = Base64::decode(base64Buffer);
                    if (!decoded.empty()) {
                        callback(decoded, TtsError::None);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Base64 decode error (final): " << e.what() << std::endl;
                    result = TtsError::DecodingError;
                }
            }

            // Check response status
            if (response.status_code == 0) {
                result = TtsError::NetworkError;
            } else if (response.status_code < 200 || response.status_code >= 300) {
                std::cerr << "HTTP Error: " << response.status_code << std::endl;
                result = TtsError::InvalidResponse;
            }

            if (mCancelled.load()) {
                result = TtsError::RequestCancelled;
            }

        } catch (const std::exception& e) {
            std::cerr << "TTS synthesis error: " << e.what() << std::endl;
            result = TtsError::NetworkError;
        }

        mSynthesizing.store(false);

        // Send final callback with empty data to signal completion
        if (result != TtsError::None) {
            callback(std::vector<uint8_t>{}, result);
        } else {
            callback(std::vector<uint8_t>{}, TtsError::None);
        }

        return result;
    }

    void cancel() {
        mCancelled.store(true);
    }

    bool isSynthesizing() const {
        return mSynthesizing.load();
    }

private:
    std::string buildRequestBody(const std::string& text,
                                 const std::map<std::string, std::string>& additionalParams) {
        // Build a simple JSON request body
        std::string body = "{\"text\":\"" + escapeJson(text) + "\"";

        for (const auto& [key, value] : additionalParams) {
            body += ",\"" + escapeJson(key) + "\":\"" + escapeJson(value) + "\"";
        }

        body += "}";
        return body;
    }

    std::string escapeJson(const std::string& str) {
        std::string result;
        result.reserve(str.size());

        for (char c : str) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (c < 0x20) {
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04x", c);
                        result += buf;
                    } else {
                        result += c;
                    }
            }
        }

        return result;
    }

    std::string getApiUrl() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mConfig.apiUrl;
    }

    long getTimeoutMs() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mConfig.timeoutMs;
    }

    TtsConfig mConfig;
    std::atomic<bool> mCancelled;
    std::atomic<bool> mSynthesizing;
    mutable std::mutex mMutex;
};

// TtsEngine implementation
TtsEngine::TtsEngine() : mImpl(std::make_unique<Impl>()) {}

TtsEngine::TtsEngine(const TtsConfig& config)
    : mImpl(std::make_unique<Impl>(config)) {}

TtsEngine::~TtsEngine() = default;

void TtsEngine::setConfig(const TtsConfig& config) {
    mImpl->setConfig(config);
}

TtsConfig TtsEngine::getConfig() const {
    return mImpl->getConfig();
}

TtsError TtsEngine::synthesize(const std::string& text,
                               AudioChunkCallback callback,
                               const std::map<std::string, std::string>& additionalParams) {
    return mImpl->synthesize(text, callback, additionalParams);
}

void TtsEngine::cancel() {
    mImpl->cancel();
}

bool TtsEngine::isSynthesizing() const {
    return mImpl->isSynthesizing();
}

} // namespace utils

