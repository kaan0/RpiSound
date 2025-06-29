#include <charconv>
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
    return true;
}

const types::SoundSample& PcmLoader::getSample(const std::string_view sampleName) const {
    auto it = m_samples.find(sampleName);
    if (it != m_samples.end()) {
        return it->second;
    } else {
        return types::SoundSample{};  // Return an empty sample if not found
    }
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
        std::cerr << "Failed to open PCM file: " << filePath << std::endl;
        return false;
    }

    // 1) Read header more efficiently until marker is found
    std::string header;
    header.reserve(512);
    std::string line;
    bool markerFound = false;

    while (std::getline(file, line)) {
        if (line.find(kDataMarker) != std::string::npos) {
            markerFound = true;
            break;
        }
        header += line + '\n';

        // Safety check for header size
        if (header.size() > MAX_HEADER_SIZE) {
            std::cerr << "Header too large in " << filePath << std::endl;
            return false;
        }
    }

    if (!markerFound) {
        std::cerr << "PCM DATA marker not found in " << filePath << std::endl;
        return false;
    }

    // 2) Parse fields via string_view + from_chars
    std::string_view hv{header};
    std::cout << "Header: " << hv << std::endl;
    auto parseField = [&](const char* key, auto& out) {
        auto keyv = std::string_view{key};
        auto pos = hv.find(keyv);
        if (pos == hv.npos)
            return false;
        pos += keyv.size();
        auto end = hv.find('|', pos);
        auto tok = hv.substr(pos, end - pos);
        std::cout << "Parsing field: " << key << " with value: " << tok << std::endl;

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
        std::cerr << "Missing or invalid 'name' field in " << filePath << std::endl;
        return false;
    }

    if (!parseField(kKeySampleRate, sample.metadata.sampleRate)) {
        std::cerr << "Missing or invalid 'samplerate' field in " << filePath << std::endl;
        return false;
    }

    if (!parseField(kKeyChannels, sample.metadata.channels)) {
        std::cerr << "Missing or invalid 'channels' field in " << filePath << std::endl;
        return false;
    }

    if (!parseField(kKeyFrames, sample.metadata.frames)) {
        std::cerr << "Missing or invalid 'frames' field in " << filePath << std::endl;
        return false;
    }

    // Parse sample width for validation
    uint32_t sampleWidth = 0;
    if (!parseField(kKeySampleWidth, sampleWidth)) {
        std::cerr << "Missing or invalid 'samplewidth' field in " << filePath << std::endl;
        return false;
    }

    // Validate parsed values
    if (sample.metadata.sampleRate < MIN_SAMPLE_RATE || sample.metadata.sampleRate > MAX_SAMPLE_RATE) {
        std::cerr << "Invalid sample rate: " << sample.metadata.sampleRate << " in " << filePath << std::endl;
        return false;
    }

    if (sample.metadata.channels == 0 || sample.metadata.channels > MAX_CHANNELS) {
        std::cerr << "Invalid channel count: " << sample.metadata.channels << " in " << filePath << std::endl;
        return false;
    }

    if (sample.metadata.frames == 0) {
        std::cerr << "Invalid frame count: " << sample.metadata.frames << " in " << filePath << std::endl;
        return false;
    }

    if (sampleWidth != sizeof(types::audio_t)) {
        std::cerr << "Unsupported sample width " << sampleWidth << " (expected " << sizeof(types::audio_t) << ") in "
                  << filePath << std::endl;
        return false;
    }

    // 3) Validate total data size before allocation
    size_t totalSamples = sample.metadata.frames * sample.metadata.channels;
    if (totalSamples > MAX_AUDIO_DATA_SIZE / sizeof(types::audio_t)) {
        std::cerr << "Audio data too large: " << totalSamples << " samples in " << filePath << std::endl;
        return false;
    }

    // 4) Read interleaved audio data
    sample.audioData.resize(totalSamples);
    auto expectedBytes = totalSamples * sizeof(types::audio_t);

    file.read(reinterpret_cast<char*>(sample.audioData.data()), expectedBytes);

    // Check if we read the expected amount of data
    auto bytesRead = file.gcount();
    if (static_cast<size_t>(bytesRead) != expectedBytes) {
        std::cerr << "Partial read: expected " << expectedBytes << " bytes, got " << bytesRead << " in " << filePath
                  << std::endl;
        return false;
    }

    return true;
}
