#ifndef _PCM_CONVERTER_HPP__
#define _PCM_CONVERTER_HPP__

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "iaudio_parser.hpp"

class PCMConverter {
public:
    PCMConverter() {};
    ~PCMConverter() {};
    bool load(const std::string_view& filePath);
    bool convertToHwPCM(AudioFormat hwAudioFormat);
    std::shared_ptr<PCMData> getData() const;

private:
    std::unique_ptr<IAudioParser> m_parser_;
    std::shared_ptr<PCMData> converted_pcm_data_;
};

#endif // _PCM_CONVERTER_HPP__
