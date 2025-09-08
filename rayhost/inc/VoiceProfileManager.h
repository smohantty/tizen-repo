#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <fstream>

namespace rayhost {

class VoiceProfileManager {
public:
    struct VoiceProfile {
        std::string mName;
        std::vector<short> mAudioData;
        std::string mFilePath;
        double mConfidence;

        VoiceProfile(const std::string& name)
            : mName(name), mConfidence(0.0) {}
    };

    VoiceProfileManager();
    ~VoiceProfileManager();

    // Profile management
    bool registerProfile(const std::string& profileName, size_t recordingDurationMs = 5000);
    bool verifyProfile(const std::string& profileName, size_t recordingDurationMs = 3000);
    bool deleteProfile(const std::string& profileName);
    std::vector<std::string> listProfiles() const;
    bool profileExists(const std::string& profileName) const;

    // Audio processing
    void startAudioCapture();
    void stopAudioCapture();
    bool isCapturing() const;

    // File operations
    bool saveProfileToFile(const std::string& profileName);
    bool loadProfileFromFile(const std::string& profileName);
    bool loadAllProfiles();

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace rayhost
