#include <string>

#include "rpi_sound/pcm_converter.hpp"
#include "rpi_sound/wav_parser.hpp"

bool PCMConverter::load(const std::string_view& filePath) {
    if (filePath.ends_with(".wav")) {
        m_parser_ = std::make_unique<WavParser>();
    } else {
        return false; // Unsupported format
    }

    if (!m_parser_->load(filePath)) {
        return false;
    }
    // TODO: do this after conversion
    converted_pcm_data_ = std::move(m_parser_->getPCMData());

    return true;
}

bool PCMConverter::convertToHwPCM(AudioFormat hwAudioFormat) {
    if (hwAudioFormat == m_parser_->getAudioFormat()) {
        // No need to do the conversion
        converted_pcm_data_ = std::move(m_parser_->getPCMData());
        return true;
    }

    return false;
}

std::shared_ptr<PCMData> PCMConverter::getData() const {
    return converted_pcm_data_;
}


