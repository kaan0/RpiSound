#ifndef _WAV_PARSER_HPP__
#define _WAV_PARSER_HPP__

#include "iaudio_parser.hpp"

class WavParser : public IAudioParser {
public:
    WavParser();
    ~WavParser() override;

    bool load(const std::string_view& filePath) override;
    std::shared_ptr<PCMData> getPCMData() const override;
    AudioFormat getAudioFormat() const override;

private:
    std::shared_ptr<PCMData> pcm_data_;
};

#endif // _WAV_PARSER_HPP__
