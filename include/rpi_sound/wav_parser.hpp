#ifndef _WAV_PARSER_HPP__
#define _WAV_PARSER_HPP__

#include "iaudio_parser.hpp"


class WavParser : public IAudioParser {
public:
    WavParser();
    ~WavParser() override;

    bool load(const std::string& filePath) override;
    std::vector<uint8_t> getPCMData() const override;
    int getSampleRate() const override;
    int getChannels() const override;

private:
    std::vector<uint8_t> pcm_data_;
    int sample_rate_ = 0;
    int channels_ = 0;
};

#endif // _WAV_PARSER_HPP__
