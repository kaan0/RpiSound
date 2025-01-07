#include "rpi_sound/wav_parser.hpp"

WavParser::WavParser() {

}

WavParser::~WavParser() {
    
}

bool WavParser::load(const std::string& filePath) {
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