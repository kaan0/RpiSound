#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>

#include "rpi_sound/wav_parser.hpp"

namespace {
    constexpr char kRiffHeader[] = "RIFF";
    constexpr char kWaveHeader[] = "WAVE";
    constexpr char kFmtHeader[] = "fmt";
    constexpr char kDataHeader[] = "data";

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
        uint16_t subExtensionSize;
        uint16_t validBitRate;
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
        kWaveFormatExtensible = 65534
    };
}

WavParser::WavParser() {
    std::cout << "Wav parser created!\r\n";
}

WavParser::~WavParser() {
    
}

bool WavParser::load(const std::string_view& filePath) {
    std::ifstream file(filePath.data(), std::ios::binary);
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

    switch (chunkFormat.audioFormat) {
        case WaveFormat::kWaveFormatExtensible:
            ExtendedChunkFormat extendedChunkFormat;

            file.read(reinterpret_cast<char*>(&extendedChunkFormat), sizeof(extendedChunkFormat));

            if((extendedChunkFormat.subFormat[0] == WaveFormat::kIeeeFloat) ||
                extendedChunkFormat.subFormat[0] == WaveFormat::kPCM) {
                chunkFormat.audioFormat = static_cast<WaveFormat>(extendedChunkFormat.subFormat[0]);
            } else {
                std::cout << "Unknown format\r\n";
                return false;
            }
            break;

        case WaveFormat::kPCM:
        case WaveFormat::kIeeeFloat:
            break;

        default:
            return false;
    }

    DataHeader dataHeader;
    constexpr auto header_size = std::size(dataHeader.data);
    do {
        file.read(reinterpret_cast<char*>(&dataHeader), sizeof(dataHeader));
        if (std::memcmp(dataHeader.data, kDataHeader, header_size) == 0) {
            break;
        }
        file.seekg(-(sizeof(dataHeader) - 1), std::ios::cur);
    } while(file.peek() != std::ifstream::traits_type::eof());

    auto currentPos = file.tellg();
    file.seekg(currentPos, std::ios::end);
    auto endPos = file.tellg();
    file.seekg(currentPos, std::ios::beg);
    if (dataHeader.dataSize > (endPos - currentPos)) {
        std::cout << "Reported: " << dataHeader.dataSize << " Actual: " << (endPos - currentPos);
        return false;
    }

    auto format = AudioFormat(chunkFormat.sampleRate,
                              chunkFormat.numChannels,
                              chunkFormat.audioFormat == WaveFormat::kIeeeFloat,
                              chunkFormat.bitsPerSample);

    pcm_data_ = std::make_shared<PCMData>();
    pcm_data_->data = std::vector<uint8_t>(dataHeader.dataSize);
    pcm_data_->format = format;

    if(!file.read(reinterpret_cast<char*>(pcm_data_->data.data()), dataHeader.dataSize)) {
        return false;
    }

    return true;
}

std::shared_ptr<PCMData> WavParser::getPCMData() const {
    return pcm_data_;
}

AudioFormat WavParser::getAudioFormat() const {
    return pcm_data_->format;
}