#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <variant>
#include <string>
#include <optional>

// ---------------------- Message Types ----------------------

struct PrintMessage {
    std::string text;
};

struct ComputeMessage {
    int value;
};

struct ShutdownMessage {};

// Union of all message types
using Message = std::variant<PrintMessage, ComputeMessage, ShutdownMessage>;

// ---------------------- Thread-Safe Queue ----------------------

class MessageQueue {
public:
    void push(Message msg) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            queue_.push(std::move(msg));
        }
        cv_.notify_one();
    }

    std::optional<Message> pop() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return !queue_.empty(); });

        Message msg = std::move(queue_.front());
        queue_.pop();
        return msg;
    }

private:
    std::queue<Message> queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
};

// ---------------------- Worker ----------------------

class Worker {
public:
    Worker() : running_(true), worker_(&Worker::run, this) {}

    ~Worker() {
        send(ShutdownMessage{}); // graceful shutdown
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    template <typename T>
    void send(T msg) {
        queue_.push(std::move(msg));
    }

private:
    void run() {
        while (running_) {
            auto msgOpt = queue_.pop();
            if (!msgOpt) continue;

            std::visit([this](auto&& msg) {
                using T = std::decay_t<decltype(msg)>;

                if constexpr (std::is_same_v<T, PrintMessage>) {
                    std::cout << "[Worker] Print: " << msg.text << "\n";
                }
                else if constexpr (std::is_same_v<T, ComputeMessage>) {
                    std::cout << "[Worker] Compute: " << msg.value
                              << "^2 = " << (msg.value * msg.value) << "\n";
                }
                else if constexpr (std::is_same_v<T, ShutdownMessage>) {
                    std::cout << "[Worker] Shutting down...\n";
                    running_ = false;
                }
            }, *msgOpt);
        }
    }

    bool running_;
    MessageQueue queue_;
    std::thread worker_;
};

// ---------------------- Example Usage ----------------------

int main() {
    Worker worker;

    worker.send(PrintMessage{"Hello from main thread!"});
    worker.send(ComputeMessage{7});
    worker.send(PrintMessage{"Done computing"});

    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0; // Worker shuts down in destructor
}
