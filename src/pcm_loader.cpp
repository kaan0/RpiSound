#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

#include "rpi_sound/pcm_loader.hpp"

bool PcmLoader::load(const std::string& instrumentFolder) {
    auto rootPath{std::filesystem::path(kSoundDirectory) / std::filesystem::path(instrumentFolder)};
    if (!std::filesystem::exists(rootPath) || !std::filesystem::is_directory(rootPath)) {
        std::cout << "Instrument folder does not exist or is not a directory: " << rootPath << std::endl;
        return false;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(rootPath)) {
            if (entry.is_regular_file() && entry.path().extension() == kFileExtension) {
                types::SoundSample sample;
                if (parseSample(entry.path().string(), sample)) {
                    m_samples[entry.path().stem().string()] = std::move(sample);
                } else {
                    std::cerr << "Failed to load sound sample: " << entry.path() << std::endl;
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return false;  // Error during directory iteration
    }
}

// Get the sound sample for the specified file path
bool PcmLoader::parseSample(const std::filesystem::path& filePath, types::SoundSample& sample) const {
    // Open the PCM file
    std::ifstream file{filePath, std::ios::binary};
    if (!file.good()) {
        std::cerr << "Failed to open PCM file: " << filePath << std::endl;
        return false;  // File could not be opened
    }
    // TODO: Implement the parsing logic for the PCM file header and audio data
    return true;
}
