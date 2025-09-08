#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <termios.h>
#include <unistd.h>
#include <iomanip>

namespace rayhost {

// ANSI Color codes for terminal styling
namespace Colors {
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string DIM = "\033[2m";
    const std::string UNDERLINE = "\033[4m";

    // Text colors
    const std::string BLACK = "\033[30m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";

    // Bright colors
    const std::string BRIGHT_BLACK = "\033[90m";
    const std::string BRIGHT_RED = "\033[91m";
    const std::string BRIGHT_GREEN = "\033[92m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_BLUE = "\033[94m";
    const std::string BRIGHT_MAGENTA = "\033[95m";
    const std::string BRIGHT_CYAN = "\033[96m";
    const std::string BRIGHT_WHITE = "\033[97m";

    // Background colors
    const std::string BG_BLACK = "\033[40m";
    const std::string BG_RED = "\033[41m";
    const std::string BG_GREEN = "\033[42m";
    const std::string BG_YELLOW = "\033[43m";
    const std::string BG_BLUE = "\033[44m";
    const std::string BG_MAGENTA = "\033[45m";
    const std::string BG_CYAN = "\033[46m";
    const std::string BG_WHITE = "\033[47m";
}

// UI Helper functions
class UIHelper {
public:
    static void clearScreen() {
        std::cout << "\033[2J\033[H";
    }

    static void printHeader() {
        std::cout << Colors::BRIGHT_CYAN << Colors::BOLD;
        std::cout << "+==============================================================+\n";
        std::cout << "|                                                              |\n";
        std::cout << "|" << std::setw(30) << "" << Colors::BRIGHT_WHITE << "🎤 RayHost Voice Manager 🎤" << Colors::BRIGHT_CYAN << std::setw(30) << "" << "|\n";
        std::cout << "|" << std::setw(25) << "" << Colors::BRIGHT_YELLOW << "Advanced Voice Profile System" << Colors::BRIGHT_CYAN << std::setw(25) << "" << "|\n";
        std::cout << "|                                                              |\n";
        std::cout << "+==============================================================+\n";
        std::cout << Colors::RESET;
    }

    static void printSeparator() {
        std::cout << Colors::BRIGHT_BLUE << "----------------------------------------------------------------\n" << Colors::RESET;
    }

    static void printSuccess(const std::string& message) {
        std::cout << Colors::BRIGHT_GREEN << "✓ " << message << Colors::RESET << std::endl;
    }

    static void printError(const std::string& message) {
        std::cout << Colors::BRIGHT_RED << "✗ " << message << Colors::RESET << std::endl;
    }

    static void printWarning(const std::string& message) {
        std::cout << Colors::BRIGHT_YELLOW << "⚠ " << message << Colors::RESET << std::endl;
    }

    static void printInfo(const std::string& message) {
        std::cout << Colors::BRIGHT_CYAN << "ℹ " << message << Colors::RESET << std::endl;
    }

    static void printProgress(const std::string& message) {
        std::cout << Colors::BRIGHT_MAGENTA << "⟳ " << message << Colors::RESET << std::endl;
    }

    static void printMenuOption(int number, const std::string& text) {
        std::cout << Colors::BRIGHT_WHITE << "  " << number << ". " << Colors::RESET << text << std::endl;
    }

    static void printPrompt(const std::string& message) {
        std::cout << Colors::BRIGHT_GREEN << "➤ " << message << Colors::RESET;
    }

    static void printLoadingDots(int seconds) {
        for (int i = 0; i < seconds * 2; ++i) {
            std::cout << Colors::BRIGHT_MAGENTA << "●" << Colors::RESET;
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cout << std::endl;
    }
};

class VoiceProfileManager {
public:
    VoiceProfileManager() = default;
    ~VoiceProfileManager() = default;

    // Placeholder functions for voice profile operations
    bool registerVoiceProfile(const std::string& profileName) {
        UIHelper::clearScreen();
        UIHelper::printHeader();

        std::cout << Colors::BRIGHT_CYAN << Colors::BOLD << "\n🎤 VOICE PROFILE REGISTRATION 🎤\n" << Colors::RESET;
        UIHelper::printSeparator();

        UIHelper::printInfo("Registering voice profile: " + Colors::BRIGHT_WHITE + profileName + Colors::RESET);
        UIHelper::printInfo("You will be asked to record 3 sentences for voice training.");

        // Define the 3 sentences for voice training
        std::vector<std::string> trainingSentences = {
            "The quick brown fox jumps over the lazy dog.",
            "Hello, my name is " + profileName + " and I am registering my voice.",
            "This is my third and final sentence for voice profile training."
        };

        std::vector<std::vector<short>> recordedAudio; // Store recorded audio data

        for (int i = 0; i < 3; ++i) {
            std::cout << "\n" << Colors::BRIGHT_MAGENTA << "📝 SENTENCE " << (i + 1) << " OF 3" << Colors::RESET << std::endl;
            UIHelper::printSeparator();

            std::cout << Colors::BRIGHT_WHITE << "Please read this sentence clearly:" << Colors::RESET << std::endl;
            std::cout << Colors::BRIGHT_YELLOW << "\"" << trainingSentences[i] << "\"" << Colors::RESET << std::endl;

            std::cout << "\n" << Colors::BRIGHT_CYAN << "Press Enter when you're ready to record..." << Colors::RESET;
            std::cin.get();

            std::cout << "\n" << Colors::BRIGHT_YELLOW << "📢 Get ready to speak in:" << Colors::RESET << std::endl;
            for (int j = 3; j > 0; --j) {
                std::cout << Colors::BRIGHT_RED << "   " << j << "..." << Colors::RESET << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            UIHelper::printProgress("Recording sentence " + std::to_string(i + 1) + "...");

            // TODO: Implement actual audio recording
            // This is where you would integrate with your audio capture system
            std::vector<short> audioData = recordAudio(5); // Record for 5 seconds
            recordedAudio.push_back(audioData);

            UIHelper::printSuccess("Sentence " + std::to_string(i + 1) + " recorded successfully!");

            if (i < 2) {
                std::cout << "\n" << Colors::BRIGHT_BLUE << "Press Enter to continue to next sentence..." << Colors::RESET;
                std::cin.get();
            }
        }

        UIHelper::printProgress("Processing voice characteristics from all recordings...");
        UIHelper::printLoadingDots(3);

        // TODO: Implement voice feature extraction and storage
        bool success = processAndStoreVoiceProfile(profileName, recordedAudio, trainingSentences);

        if (success) {
            UIHelper::printSuccess("Voice profile '" + profileName + "' registered successfully!");
            std::cout << Colors::BRIGHT_GREEN << "🎉 Your voice has been trained with 3 sentences!" << Colors::RESET << std::endl;
            mProfiles[profileName] = true;
            return true;
        } else {
            UIHelper::printError("Failed to process voice profile. Please try again.");
            return false;
        }
    }

    bool verifyVoiceProfile(const std::string& profileName) {
        UIHelper::clearScreen();
        UIHelper::printHeader();

        std::cout << Colors::BRIGHT_CYAN << Colors::BOLD << "\n🔐 VOICE PROFILE VERIFICATION 🔐\n" << Colors::RESET;
        UIHelper::printSeparator();

        if (mProfiles.find(profileName) == mProfiles.end()) {
            UIHelper::printError("Profile '" + profileName + "' not found!");
            return false;
        }

        UIHelper::printInfo("Verifying voice profile: " + Colors::BRIGHT_WHITE + profileName + Colors::RESET);
        UIHelper::printInfo("Please speak your passphrase when prompted...");

        std::cout << "\n" << Colors::BRIGHT_YELLOW << "📢 Get ready to speak in:" << Colors::RESET << std::endl;
        for (int i = 3; i > 0; --i) {
            std::cout << Colors::BRIGHT_RED << "   " << i << "..." << Colors::RESET << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        UIHelper::printProgress("Recording audio for 3 seconds");
        UIHelper::printLoadingDots(1);

        UIHelper::printProgress("Comparing with stored voice characteristics");
        UIHelper::printLoadingDots(2);

        // Simulate verification result (random for demo)
        bool verified = (rand() % 2) == 1;

        if (verified) {
            UIHelper::printSuccess("Voice verification successful! Access granted.");
            std::cout << Colors::BRIGHT_GREEN << "🎉 Welcome back, " << profileName << "!" << Colors::RESET << std::endl;
        } else {
            UIHelper::printError("Voice verification failed! Access denied.");
            std::cout << Colors::BRIGHT_RED << "🚫 Authentication failed. Please try again." << Colors::RESET << std::endl;
        }

        return verified;
    }

    void listProfiles() {
        UIHelper::clearScreen();
        UIHelper::printHeader();

        std::cout << Colors::BRIGHT_CYAN << Colors::BOLD << "\n📋 REGISTERED VOICE PROFILES 📋\n" << Colors::RESET;
        UIHelper::printSeparator();

        if (mProfiles.empty()) {
            UIHelper::printWarning("No voice profiles registered.");
            std::cout << Colors::BRIGHT_YELLOW << "💡 Use option 1 to register your first voice profile!" << Colors::RESET << std::endl;
        } else {
            std::cout << Colors::BRIGHT_GREEN << "Found " << mProfiles.size() << " registered profile(s):\n" << Colors::RESET;
            int index = 1;
            for (const auto& profile : mProfiles) {
                std::cout << Colors::BRIGHT_WHITE << "  " << index << ". " << Colors::BRIGHT_CYAN << "👤 " << profile.first << Colors::RESET << std::endl;
                index++;
            }
        }
    }

    bool deleteProfile(const std::string& profileName) {
        UIHelper::clearScreen();
        UIHelper::printHeader();

        std::cout << Colors::BRIGHT_CYAN << Colors::BOLD << "\n🗑️  DELETE VOICE PROFILE 🗑️\n" << Colors::RESET;
        UIHelper::printSeparator();

        auto it = mProfiles.find(profileName);
        if (it != mProfiles.end()) {
            UIHelper::printWarning("Are you sure you want to delete profile: " + Colors::BRIGHT_WHITE + profileName + Colors::RESET + "?");
            std::cout << Colors::BRIGHT_RED << "⚠️  This action cannot be undone!" << Colors::RESET << std::endl;

            UIHelper::printPrompt("Type 'yes' to confirm deletion: ");
            std::string confirmation;
            std::getline(std::cin, confirmation);

            if (confirmation == "yes" || confirmation == "YES") {
                mProfiles.erase(it);
                UIHelper::printSuccess("Profile '" + profileName + "' deleted successfully!");
                return true;
            } else {
                UIHelper::printInfo("Deletion cancelled.");
                return false;
            }
        } else {
            UIHelper::printError("Profile '" + profileName + "' not found!");
            return false;
        }
    }

private:
    std::map<std::string, bool> mProfiles;

    // Skeleton functions for audio recording and processing
    std::vector<short> recordAudio(int durationSeconds) {
        // TODO: Implement actual audio recording
        // This function should:
        // 1. Initialize audio capture (microphone)
        // 2. Record audio for the specified duration
        // 3. Return the raw audio data as vector<short>

        std::cout << Colors::BRIGHT_MAGENTA << "🎙️  Recording audio for " << durationSeconds << " seconds..." << Colors::RESET << std::endl;

        // Simulate recording time
        UIHelper::printLoadingDots(durationSeconds);

        // Return dummy audio data (in real implementation, this would be actual audio samples)
        std::vector<short> dummyAudio(durationSeconds * 16000, 0); // 16kHz sample rate
        return dummyAudio;
    }

    bool processAndStoreVoiceProfile(const std::string& profileName,
                                   const std::vector<std::vector<short>>& audioData,
                                   const std::vector<std::string>& sentences) {
        // TODO: Implement voice feature extraction and storage
        // This function should:
        // 1. Extract voice features from the recorded audio (MFCC, pitch, formants, etc.)
        // 2. Create a voice model/template
        // 3. Store the voice profile data (file, database, etc.)
        // 4. Return true if successful, false otherwise

        std::cout << Colors::BRIGHT_CYAN << "🔬 Extracting voice features..." << Colors::RESET << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::cout << Colors::BRIGHT_CYAN << "📊 Analyzing voice characteristics..." << Colors::RESET << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::cout << Colors::BRIGHT_CYAN << "💾 Storing voice profile data..." << Colors::RESET << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // TODO: Implement actual voice processing:
        // - Extract MFCC features
        // - Calculate pitch and formant frequencies
        // - Create voice template/model
        // - Save to file or database

        // For now, just simulate success
        return true;
    }

    std::vector<float> extractVoiceFeatures(const std::vector<short>& audioData) {
        // TODO: Implement voice feature extraction
        // This function should extract features like:
        // - MFCC (Mel-frequency cepstral coefficients)
        // - Pitch/F0 (fundamental frequency)
        // - Formant frequencies (F1, F2, F3)
        // - Spectral features
        // - Prosodic features (rhythm, stress patterns)

        std::vector<float> features;
        // TODO: Add actual feature extraction logic here
        return features;
    }

    bool saveVoiceProfile(const std::string& profileName, const std::vector<float>& features) {
        // TODO: Implement voice profile storage
        // This function should save the voice profile to:
        // - A file (JSON, binary, etc.)
        // - A database
        // - Encrypted storage for security

        // TODO: Add actual storage logic here
        return true;
    }
};

class MenuApp {
public:
    MenuApp() : mProfileManager() {}

    void run() {
        while (true) {
            displayMenu();
            int choice = getMenuChoice();

            switch (choice) {
                case 1:
                    handleRegisterProfile();
                    break;
                case 2:
                    handleVerifyProfile();
                    break;
                case 3:
                    handleListProfiles();
                    break;
                case 4:
                    handleDeleteProfile();
                    break;
                case 5:
                    UIHelper::clearScreen();
                    std::cout << Colors::BRIGHT_CYAN << Colors::BOLD;
                    std::cout << "+==============================================================+\n";
                    std::cout << "|                                                              |\n";
                    std::cout << "|" << std::setw(20) << "" << Colors::BRIGHT_WHITE << "👋 Thank you for using RayHost! 👋" << Colors::BRIGHT_CYAN << std::setw(20) << "" << "|\n";
                    std::cout << "|" << std::setw(25) << "" << Colors::BRIGHT_YELLOW << "Voice Profile Manager" << Colors::BRIGHT_CYAN << std::setw(25) << "" << "|\n";
                    std::cout << "|                                                              |\n";
                    std::cout << "+==============================================================+\n";
                    std::cout << Colors::RESET;
                    return;
                default:
                    UIHelper::printError("Invalid choice! Please try again.");
                    break;
            }

            if (choice >= 1 && choice <= 4) {
                std::cout << "\n" << Colors::BRIGHT_BLUE << "Press Enter to continue (or Escape to exit)..." << Colors::RESET;
                char ch = getCharInput();
                if (ch == 27) { // Escape key
                    UIHelper::clearScreen();
                    std::cout << Colors::BRIGHT_CYAN << Colors::BOLD;
                    std::cout << "+==============================================================+\n";
                    std::cout << "|                                                              |\n";
                    std::cout << "|" << std::setw(20) << "" << Colors::BRIGHT_WHITE << "👋 Thank you for using RayHost! 👋" << Colors::BRIGHT_CYAN << std::setw(20) << "" << "|\n";
                    std::cout << "|" << std::setw(25) << "" << Colors::BRIGHT_YELLOW << "Voice Profile Manager" << Colors::BRIGHT_CYAN << std::setw(25) << "" << "|\n";
                    std::cout << "|                                                              |\n";
                    std::cout << "+==============================================================+\n";
                    std::cout << Colors::RESET;
                    return;
                }
            }
        }
    }

private:
    void displayMenu() {
        UIHelper::clearScreen();
        UIHelper::printHeader();

        std::cout << Colors::BRIGHT_CYAN << Colors::BOLD << "\n🎯 MAIN MENU 🎯\n" << Colors::RESET;
        UIHelper::printSeparator();

        UIHelper::printMenuOption(1, "🎤 Register Voice Profile");
        UIHelper::printMenuOption(2, "🔐 Verify Voice Profile");
        UIHelper::printMenuOption(3, "📋 List Registered Profiles");
        UIHelper::printMenuOption(4, "🗑️  Delete Voice Profile");
        UIHelper::printMenuOption(5, "🚪 Exit Application");

        UIHelper::printSeparator();
        UIHelper::printPrompt("Enter your choice (1-5), 'q' to quit, or Escape to exit: ");
        std::cout.flush(); // Ensure the prompt is displayed
    }

    int getMenuChoice() {
        // Use single character input to detect Escape key
        char ch = getCharInput();

        // Debug: print the character code
        // std::cout << "Debug: Character code = " << (int)ch << std::endl;

        // Check for Escape key (ASCII 27)
        if (ch == 27) {
            return 5; // Exit option
        }

        // Check for 'q' or 'Q' to exit
        if (ch == 'q' || ch == 'Q') {
            return 5; // Exit option
        }

        // Check for numeric choices (1-5)
        if (ch >= '1' && ch <= '5') {
            return ch - '0'; // Convert char to int
        }

        // If it's not a valid choice, return -1
        return -1;
    }

    char getCharInput() {
        struct termios oldt, newt;
        char ch;

        // Get current terminal settings
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;

        // Disable canonical mode and echo
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        // Read a single character
        ch = getchar();

        // Restore terminal settings
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

        return ch;
    }

    void handleRegisterProfile() {
        UIHelper::clearScreen();
        UIHelper::printHeader();

        std::cout << Colors::BRIGHT_CYAN << Colors::BOLD << "\n🎤 REGISTER NEW VOICE PROFILE 🎤\n" << Colors::RESET;
        UIHelper::printSeparator();

        UIHelper::printPrompt("Enter profile name: ");
        std::string profileName;
        std::getline(std::cin, profileName);

        if (profileName.empty()) {
            UIHelper::printError("Profile name cannot be empty!");
            return;
        }

        mProfileManager.registerVoiceProfile(profileName);
    }

    void handleVerifyProfile() {
        UIHelper::clearScreen();
        UIHelper::printHeader();

        std::cout << Colors::BRIGHT_CYAN << Colors::BOLD << "\n🔐 VERIFY VOICE PROFILE 🔐\n" << Colors::RESET;
        UIHelper::printSeparator();

        UIHelper::printPrompt("Enter profile name to verify: ");
        std::string profileName;
        std::getline(std::cin, profileName);

        if (profileName.empty()) {
            UIHelper::printError("Profile name cannot be empty!");
            return;
        }

        mProfileManager.verifyVoiceProfile(profileName);
    }

    void handleListProfiles() {
        mProfileManager.listProfiles();
    }

    void handleDeleteProfile() {
        UIHelper::clearScreen();
        UIHelper::printHeader();

        std::cout << Colors::BRIGHT_CYAN << Colors::BOLD << "\n🗑️  DELETE VOICE PROFILE 🗑️\n" << Colors::RESET;
        UIHelper::printSeparator();

        UIHelper::printPrompt("Enter profile name to delete: ");
        std::string profileName;
        std::getline(std::cin, profileName);

        if (profileName.empty()) {
            UIHelper::printError("Profile name cannot be empty!");
            return;
        }

        mProfileManager.deleteProfile(profileName);
    }

    VoiceProfileManager mProfileManager;
};

} // namespace rayhost

int main() {
    // Seed random number generator for demo verification results
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    try {
        rayhost::MenuApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
