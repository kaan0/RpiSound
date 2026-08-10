#pragma once

#include <filesystem>
#include <unordered_map>

#include "interfaces/isound_loader.hpp"

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
    bool load(std::string_view instrumentFolder) override;

    // Returns nullptr if the sample is not found.
    std::shared_ptr<const types::SoundSample> getSample(std::string_view sampleName) const override;

    // Get all loaded sample names
    std::vector<std::string> getSampleNames() const override;

    // Destructor
    ~PcmLoader() override = default;

private:
    // Load a sound sample from the specified file path
    bool parseSample(const std::filesystem::path& filePath, types::SoundSample& sample) const;

    std::unordered_map<std::string, std::shared_ptr<types::SoundSample>> m_samples;
};
