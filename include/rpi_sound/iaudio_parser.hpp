#ifndef _IAUDIO_PARSER_HPP__
#define _IAUDIO_PARSER_HPP__

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "audio_utils.hpp"

class IAudioParser {
public:
    virtual ~IAudioParser() = default;
    virtual bool load(const std::string_view& filePath) = 0;
    virtual std::shared_ptr<PCMData> getPCMData() const = 0;
    virtual AudioFormat getAudioFormat() const = 0;
};

#endif // _IAUDIO_PARSER_HPP__
