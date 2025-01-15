#ifndef _IAUDIO_DRIVER_HPP__
#define _IAUDIO_DRIVER_HPP__

#include <string>
#include <vector>

#include "audio_utils.hpp"

class IAudioDriver {
public:
    virtual ~IAudioDriver() = default;
    virtual bool openDevice(uint32_t card, uint32_t device, bool isOutput, HWAudioFormat& config) = 0;
    virtual bool getDeviceFormat(uint32_t card, uint32_t device, bool isOutput, HWAudioFormat& config) = 0;
    virtual void writeData(const std::vector<uint8_t>& data) = 0;
    virtual HWAudioFormat getDefaultFormat() = 0;
};

#endif // _IAUDIO_DRIVER_HPP__
