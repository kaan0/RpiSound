#ifndef _IAUDIO_PARSER_HPP__
#define _IAUDIO_PARSER_HPP__

#include <cstdint>
#include <string>
#include <vector>

class IAudioParser {
public:
    virtual ~IAudioParser() = default;
    virtual bool load(const std::string& filePath) = 0;
    virtual std::vector<uint8_t> getPCMData() const = 0;
    virtual int getSampleRate() const = 0;
    virtual int getChannels() const = 0;
};

#endif // _IAUDIO_PARSER_HPP__
