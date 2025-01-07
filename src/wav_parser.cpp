#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>

#include "rpi_sound/wav_parser.hpp"

namespace {
    constexpr auto kRiffHeader = "RIFF";
    constexpr auto kWaveHeader = "WAVE";
    constexpr auto kFmtHeader = "fmt";
    constexpr auto kDataHeader = "data";

    constexpr auto kSubFormatPCM = "\x01\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71";
    constexpr auto kSubFormatIeeeFloat = "\x03\x00\x00\x00\x10\x00\x80\x00\x00\xAA\x00\x38\x9B\x71";

    struct WavHeader {
        char riff[4];
        uint32_t riffSize;
        char wave[4];
    };
    struct ChunkHeader {
        char fmt[4];
        uint32_t chunkSize;
    };
    struct ChunkFormat {
        uint16_t audioFormat;
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
    };
    struct DataHeader {
        char data[4];
        uint32_t dataSize;
    };
    struct ExtendedChunkFormat {
        uint32_t channelMask;
        char subFormat[16];
    };

    enum WaveFormat : uint16_t {
        kUnknown = 0,
        kPCM = 1,
        kADPCM = 2,
        kIeeeFloat = 3,
        kAlaw = 6,
        kMulaw = 7,
        kDTS = 8,
        kDRM = 9,
        kMpeg = 80,
        kMpegLayer3 = 85,
        WaveFormatExtensible = 65534
    };
}

WavParser::WavParser() {
    std::cout << "Wav parser created!\r\n";
}

WavParser::~WavParser() {
    
}

bool WavParser::load(const std::string& filePath) {
    std::ifstream file{filePath, std::ios::binary};
    if (!file.good()) {
        std::cout << "File open failed!" << filePath << "\r\n";
        return false;
    }

    // Read WAV header
    WavHeader wavHeader;
    file.read(reinterpret_cast<char*>(&wavHeader), sizeof(WavHeader));

    // Read chunk header
    ChunkHeader chunkHeader;
    file.read(reinterpret_cast<char*>(&chunkHeader), sizeof(chunkHeader));

    // Read chunk format
    ChunkFormat chunkFormat;
    file.read(reinterpret_cast<char*>(&chunkFormat), sizeof(chunkFormat));

    auto isFloat = false;
    ExtendedChunkFormat extendedChunkFormat;
    switch (chunkFormat.audioFormat) {
        case WaveFormat::kPCM:
            break;

        case WaveFormat::WaveFormatExtensible:
            file.read(reinterpret_cast<char*>(&extendedChunkFormat), sizeof(extendedChunkFormat));
            if (std::memcmp(extendedChunkFormat.subFormat,
                            kSubFormatIeeeFloat,
                            sizeof(kSubFormatIeeeFloat)) == 0) {
                std::cout << "Extended float \r\n";
                isFloat = true;
            } else if (std::memcmp(extendedChunkFormat.subFormat,
                            kSubFormatPCM,
                            sizeof(kSubFormatPCM)) == 0) {
                std::cout << "Extended PCM \r\n";
            } else {
                std::cout << "Extended wrong \r\n";
                return false;
            }
            break;

        default:
            return false;
    }

    // TODO: implement wav loader
    return false;
}

std::vector<uint8_t> WavParser::getPCMData() const {
    return pcm_data_;
}

int WavParser::getSampleRate() const {
    return sample_rate_;
}

int WavParser::getChannels() const {
    return channels_;
}