#ifndef _PCM_CONVERTER_HPP__
#define _PCM_CONVERTER_HPP__

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "iaudio_parser.hpp"

class PCMConverter {
public:
    bool load(const std::string& filePath);
    std::vector<uint8_t> getPCMData() const;
    int getSampleRate() const;
    int getChannels() const;

private:
    std::unique_ptr<IAudioParser> m_parser_;

};

#endif // _PCM_CONVERTER_HPP__
