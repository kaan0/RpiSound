#include <string>

#include "pcm_converter.hpp"

#include "wav_parser.hpp"

PCMConverter::PCMConverter() {}

bool PCMConverter::load(const std::string& filePath) {
    if (filePath.ends_with(".wav")) {
        m_parser_ = std::make_unique<WavParser>();
    } else if (filePath.ends_with(".mp3")) {
        // m_parser_ = std::make_unique<Mp3Parser>();
    } else {
        return false; // Unsupported format
    }
    return m_parser_->load(filePath);
}

std::vector<uint8_t> PCMConverter::getPCMData() const {
    return {0};
}

int PCMConverter::getSampleRate() const {
    return 0;
}

int PCMConverter::getChannels() const {
    return 0;
}
