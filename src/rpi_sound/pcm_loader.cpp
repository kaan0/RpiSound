#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

#include "rpi_sound/pcm_loader.hpp"
#include "utilities/logger.hpp"

bool PcmLoader::load(const std::string& instrumentFolder) {
    auto rootPath{std::filesystem::path(kSoundDirectory) / std::filesystem::path(instrumentFolder)};
    if (!std::filesystem::exists(rootPath) || !std::filesystem::is_directory(rootPath)) {
        utilities::log.error("Instrument folder does not exist or is not a directory: {}", rootPath.string());
        return false;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(rootPath)) {
            if (entry.is_regular_file() && entry.path().extension() == kFileExtension) {
                types::SoundSample sample;
                if (parseSample(entry.path().string(), sample)) {
                    m_samples[entry.path().stem().string()] = std::move(sample);
                } else {
                    utilities::log.error("Failed to parse sound sample: {}", entry.path().string());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        utilities::log.error("Filesystem error while loading samples: {}", e.what());
        return false;  // Error during directory iteration
    }

    return true;
}

const types::SoundSample& PcmLoader::getSample(const std::string_view sampleName) const {
    // Check if the sample exists in the map
    auto it = m_samples.find(std::string(sampleName));
    if (it != m_samples.end()) {
        return it->second;
    }

    static const types::SoundSample empty_sample{};
    return empty_sample;
}

// Get the sound sample for the specified file path
bool PcmLoader::parseSample(const std::filesystem::path& filePath, types::SoundSample& sample) const {
    // Constants for validation
    static constexpr size_t MAX_HEADER_SIZE = 4096;
    static constexpr size_t MAX_AUDIO_DATA_SIZE = 100 * 1024 * 1024;  // 100MB limit
    static constexpr uint32_t MIN_SAMPLE_RATE = 8000;
    static constexpr uint32_t MAX_SAMPLE_RATE = 192000;
    static constexpr uint16_t MAX_CHANNELS = 8;

    // Open the PCM file
    std::ifstream file{filePath, std::ios::binary};
    if (!file.good()) {
        utilities::log.error("Failed to open PCM file: {}", filePath.string());
        return false;
    }

    // 1) Read header more efficiently until marker is found
    std::string header;
    header.reserve(512);
    bool markerFound = false;

    while (std::getline(file, header)) {
        if (header.find(kDataMarker) != std::string::npos) {
            markerFound = true;
            break;
        }

        // Safety check for header size
        if (header.size() > MAX_HEADER_SIZE) {
            utilities::log.error("Header too large in: {}", filePath.string());
            return false;
        }
    }

    if (!markerFound) {
        utilities::log.error("PCM DATA marker not found in: {}", filePath.string());
        return false;
    }

    // 2) Parse fields via string_view + from_chars
    std::string_view hv{header};
    auto parseField = [&](const char* key, auto& out) {
        auto keyv = std::string_view{key};
        auto pos = hv.find(keyv);
        if (pos == hv.npos)
            return false;
        pos += keyv.size();
        auto end = hv.find('|', pos);
        auto tok = hv.substr(pos, end - pos);

        if constexpr (std::is_same_v<std::decay_t<decltype(out)>, std::string>) {
            out = std::string(tok);
        } else {
            auto [ptr, ec] = std::from_chars(tok.data(), tok.data() + tok.size(), out);
            if (ec != std::errc())
                return false;
        }
        return true;
    };

    // Parse and validate each field with specific error messages
    if (!parseField(kKeyName, sample.metadata.name)) {
        utilities::log.error("Missing or invalid 'name' field in: {}", filePath.string());
        return false;
    }

    if (!parseField(kKeySampleRate, sample.metadata.sampleRate)) {
        utilities::log.error("Missing or invalid 'samplerate' field in: {}", filePath.string());
        return false;
    }

    if (!parseField(kKeyChannels, sample.metadata.channels)) {
        utilities::log.error("Missing or invalid 'channels' field in: {}", filePath.string());
        return false;
    }

    if (!parseField(kKeyFrames, sample.metadata.frames)) {
        utilities::log.error("Missing or invalid 'frames' field in: {}", filePath.string());
        return false;
    }

    // Parse sample width for validation
    uint32_t sampleWidth = 0;
    if (!parseField(kKeySampleWidth, sampleWidth)) {
        utilities::log.error("Missing or invalid 'samplewidth' field in: {}", filePath.string());
        return false;
    }

    // Validate parsed values
    if (sample.metadata.sampleRate < MIN_SAMPLE_RATE || sample.metadata.sampleRate > MAX_SAMPLE_RATE) {
        utilities::log.error("Invalid sample rate: {} in {}", sample.metadata.sampleRate, filePath.string());
        return false;
    }

    if (sample.metadata.channels == 0 || sample.metadata.channels > MAX_CHANNELS) {
        utilities::log.error("Invalid channel count: {} in {}", sample.metadata.channels, filePath.string());
        return false;
    }

    if (sample.metadata.frames == 0) {
        utilities::log.error("Invalid frame count: 0 in {}", filePath.string());
        return false;
    }

    if (sampleWidth != sizeof(types::audio_t)) {
        utilities::log.error(
            "Unsupported sample width: {} in {}, expected {}", sampleWidth, filePath.string(), sizeof(types::audio_t));
        return false;
    }

    // 3) Validate total data size before allocation
    size_t totalSamples = sample.metadata.frames * sample.metadata.channels;
    if (totalSamples > MAX_AUDIO_DATA_SIZE / sizeof(types::audio_t)) {
        utilities::log.error("Audio data too large: {} samples in {}", totalSamples, filePath.string());
        return false;
    }

    // 4) Read interleaved audio data
    sample.audioData.resize(totalSamples);
    auto expectedBytes = totalSamples * sizeof(types::audio_t);

    file.read(reinterpret_cast<char*>(sample.audioData.data()), expectedBytes);

    // Check if we read the expected amount of data
    auto bytesRead = file.gcount();
    if (static_cast<size_t>(bytesRead) != expectedBytes) {
        utilities::log.error(
            "Partial read: expected {} bytes, got {} in {}", expectedBytes, bytesRead, filePath.string());
        return false;
    }

    return true;
}
