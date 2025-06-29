#pragma once

#include <filesystem>
#include <unordered_map>

#include "isound_loader.hpp"

class PcmLoader : public ISoundLoader {
public:
    static constexpr const char* kFileExtension = ".pcm";     // File extension for PCM files
    static constexpr const char* kSoundDirectory = "sound/";  // Directory where sound files are stored

    // Keys used in the PCM file header
    static constexpr const char* kKeyName = "name:";
    static constexpr const char* kKeySampleRate = "samplerate:";
    static constexpr const char* kKeyChannels = "channels:";
    static constexpr const char* kKeySampleWidth = "samplewidth:";
    static constexpr const char* kKeyFrames = "frames:";
    static constexpr const char* kDataMarker = "PCM DATA";

    // Load sound samples from the specified instrument folder
    bool load(const std::string& instrumentFolder) override;

    // Get the loaded sound sample for the specified file path
    const types::SoundSample& getSample(const std::string_view sampleName) const override;

    // Destructor
    ~PcmLoader() override = default;

private:
    // Load a sound sample from the specified file path
    bool parseSample(const std::filesystem::path& filePath, types::SoundSample& sample) const;

    // Map to store loaded sound samples by file path
    std::unordered_map<std::string, types::SoundSample> m_samples;
};
