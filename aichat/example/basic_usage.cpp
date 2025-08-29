#include "AiChatClient.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

// Mock Gemini API simulator for testing
class MockGeminiAPI {
public:
    static void callAPI(const std::string& conversation,
                       std::function<void(const std::string&)> responseHandler) {
        std::cout << "[Gemini API] Received conversation: \"" << conversation << "\"" << std::endl;

        // Simulate API processing delay
        std::thread([conversation, responseHandler]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(800));

            // Generate mock response based on conversation
            std::string response = generateMockResponse(conversation);
            std::cout << "[Gemini API] Sending response: \"" << response << "\"" << std::endl;

            responseHandler(response);
        }).detach();
    }

private:
        static std::string generateMockResponse(const std::string& conversation) {
        // English patterns
        if (conversation.find("?") != std::string::npos) {
            return "That's a great question! Let me help you with that.";
        } else if (conversation.find("hello") != std::string::npos ||
                   conversation.find("hi") != std::string::npos) {
            return "Hello! How can I assist you today?";
        } else if (conversation.find("help") != std::string::npos) {
            return "I'm here to help! What do you need assistance with?";
        }
        // Korean patterns
        else if (conversation.find("안녕") != std::string::npos) {
            return "안녕하세요! 어떻게 도와드릴까요?";
        } else if (conversation.find("뭐") != std::string::npos ||
                   conversation.find("무엇") != std::string::npos ||
                   conversation.find("까요") != std::string::npos) {
            return "좋은 질문이네요! 도와드리겠습니다.";
        } else if (conversation.find("도움") != std::string::npos ||
                   conversation.find("도와") != std::string::npos) {
            return "네, 기꺼이 도와드리겠습니다!";
        } else {
            return "I understand. Let me provide you with a helpful response.";
        }
    }
};

void demonstrateBasicUsage() {
    std::cout << "=== Basic Usage Demonstration ===" << std::endl;

    // Create client with optimized config for latency
    aichat::AiChatClient::Config config;
    config.mTriggerTimeoutMs = 500;           // 500ms timeout for triggers
    config.mEnableSmartTriggers = true;       // Enable punctuation triggers
    config.mEnableChunking = true;            // Enable chunking
    config.mMaxBufferSize = 20;               // Max 20 sentences

    aichat::AiChatClient client(config);

    // Set up backend callback (user's Gemini integration)
    client.setBackendCallback([](const std::string& conversation, auto responseHandler) {
        MockGeminiAPI::callAPI(conversation, responseHandler);
    });

    // Set up response callback
    client.setResponseCallback([](const std::string& response) {
        std::cout << "[FINAL RESPONSE] " << response << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    });

    // Set up error callback
    client.setErrorCallback([](const std::string& error) {
        std::cerr << "[ERROR] " << error << std::endl;
    });

    // Simulate streaming sentences from ASR
    std::cout << "\n--- Scenario 1: Question with immediate trigger ---" << std::endl;
    client.streamSentence("Hello");
    client.streamSentence("how");
    client.streamSentence("are");
    client.streamSentence("you");
    client.streamSentence("today?");  // Should trigger immediate backend call

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    client.streamSentence("I");
    client.streamSentence("need");
    client.streamSentence("some");
    client.streamSentence("help");
    client.streamSentence("please.");  // Another trigger

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    client.endConversation();  // Should return cached/merged response quickly

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Reset for next scenario
    client.reset();
}

void demonstrateChunking() {
    std::cout << "\n=== Chunking Demonstration ===" << std::endl;

    aichat::AiChatClient::Config config;
    config.mChunkSize = 3;                    // Small chunks for demo
    config.mEnableChunking = true;
    config.mEnableSmartTriggers = true;

    aichat::AiChatClient client(config);

    client.setBackendCallback([](const std::string& conversation, auto responseHandler) {
        MockGeminiAPI::callAPI(conversation, responseHandler);
    });

    client.setResponseCallback([](const std::string& response) {
        std::cout << "[FINAL RESPONSE] " << response << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    });

    // Send multiple sentences to trigger chunking
    std::cout << "\n--- Scenario 2: Long conversation with chunking ---" << std::endl;
    client.streamSentence("I");
    client.streamSentence("have");
    client.streamSentence("a");           // Chunk 1 (3 sentences)

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    client.streamSentence("complex");
    client.streamSentence("technical");
    client.streamSentence("question.");  // Chunk 2 + trigger on punctuation

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    client.streamSentence("Can");
    client.streamSentence("you");
    client.streamSentence("help");
    client.streamSentence("me");
    client.streamSentence("understand");
    client.streamSentence("this?");       // Should trigger on question

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    client.endConversation();

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    client.reset();
}

void demonstrateTimeoutTrigger() {
    std::cout << "\n=== Timeout Trigger Demonstration ===" << std::endl;

    aichat::AiChatClient::Config config;
    config.mTriggerTimeoutMs = 800;           // 800ms timeout for demo
    config.mEnableSmartTriggers = false;      // Disable punctuation triggers
    config.mEnableChunking = false;           // Disable chunking

    aichat::AiChatClient client(config);

    client.setBackendCallback([](const std::string& conversation, auto responseHandler) {
        MockGeminiAPI::callAPI(conversation, responseHandler);
    });

    client.setResponseCallback([](const std::string& response) {
        std::cout << "[FINAL RESPONSE] " << response << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    });

    // Send sentences with pauses to test timeout
    std::cout << "\n--- Scenario 3: Timeout-based triggering ---" << std::endl;
    client.streamSentence("This");
    client.streamSentence("is");
    client.streamSentence("a");
    client.streamSentence("test");

    std::cout << "[INFO] Waiting for timeout trigger..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // Wait for timeout

    client.streamSentence("without");
    client.streamSentence("punctuation");

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    client.endConversation();

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

void demonstrateKoreanLanguage() {
    std::cout << "\n=== Korean Language Support Demonstration ===" << std::endl;

    aichat::AiChatClient::Config config;
    config.mEnableSmartTriggers = true;       // Enable smart triggers for Korean
    config.mTriggerTimeoutMs = 500;
    config.mLanguage = aichat::AiChatClient::Language::Korean;  // Force Korean trigger

    aichat::AiChatClient client(config);

    client.setBackendCallback([](const std::string& conversation, auto responseHandler) {
        MockGeminiAPI::callAPI(conversation, responseHandler);
    });

    client.setResponseCallback([](const std::string& response) {
        std::cout << "[FINAL RESPONSE] " << response << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    });

    // Test Korean question patterns
    std::cout << "\n--- Scenario: Korean Questions and Patterns ---" << std::endl;
    client.streamSentence("안녕하세요");
    client.streamSentence("뭐");
    client.streamSentence("하고");
    client.streamSentence("계세요?");  // Should trigger on Korean question ending

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    client.streamSentence("도움이");
    client.streamSentence("필요해요.");  // Should trigger on Korean punctuation

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    client.streamSentence("어떻게");
    client.streamSentence("할까요?");   // Should trigger on Korean question word + ending

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    client.endConversation();

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    client.reset();
}

void demonstrateAutoLanguageDetection() {
    std::cout << "\n=== Auto Language Detection Demonstration ===" << std::endl;

    aichat::AiChatClient::Config config;
    config.mEnableSmartTriggers = true;
    config.mLanguage = aichat::AiChatClient::Language::Auto;  // Auto-detect language

    aichat::AiChatClient client(config);

    client.setBackendCallback([](const std::string& conversation, auto responseHandler) {
        MockGeminiAPI::callAPI(conversation, responseHandler);
    });

    client.setResponseCallback([](const std::string& response) {
        std::cout << "[FINAL RESPONSE] " << response << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    });

    // Test mixed English and Korean in same conversation
    std::cout << "\n--- Scenario: Mixed Language Auto-Detection ---" << std::endl;
    client.streamSentence("Hello");         // English - should use English trigger
    client.streamSentence("how");
    client.streamSentence("are");
    client.streamSentence("you?");          // English question trigger

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    client.streamSentence("안녕하세요");      // Korean - should switch to Korean trigger
    client.streamSentence("뭐");
    client.streamSentence("하고");
    client.streamSentence("계세요?");        // Korean question trigger

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    client.streamSentence("Thanks");        // Back to English - should switch to English trigger
    client.streamSentence("for");
    client.streamSentence("help!");         // English punctuation trigger

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    client.endConversation();

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    client.reset();
}

void demonstratePerformanceMetrics() {
    std::cout << "\n=== Performance Metrics Demonstration ===" << std::endl;

    aichat::AiChatClient client;

    auto startTime = std::chrono::steady_clock::now();

    client.setBackendCallback([](const std::string& conversation, auto responseHandler) {
        std::cout << "[METRICS] Backend call initiated" << std::endl;
        MockGeminiAPI::callAPI(conversation, responseHandler);
    });

    client.setResponseCallback([startTime](const std::string& response) {
        auto endTime = std::chrono::steady_clock::now();
        auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::cout << "[METRICS] Total response time: " << totalTime.count() << "ms" << std::endl;
        std::cout << "[FINAL RESPONSE] " << response << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    });

    std::cout << "\n--- Scenario 4: Performance measurement ---" << std::endl;
    std::cout << "[METRICS] Starting conversation..." << std::endl;

    client.streamSentence("What");
    client.streamSentence("is");
    client.streamSentence("the");
    client.streamSentence("weather");
    client.streamSentence("like?");  // Should trigger immediately

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    client.endConversation();

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

int main() {
    std::cout << "AI Chat Client - Basic Usage Examples" << std::endl;
    std::cout << "====================================" << std::endl;

            try {
        demonstrateBasicUsage();
        demonstrateChunking();
        demonstrateKoreanLanguage();
        demonstrateAutoLanguageDetection();
        demonstrateTimeoutTrigger();
        demonstratePerformanceMetrics();

        std::cout << "\n=== All demonstrations completed successfully! ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error during demonstration: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
