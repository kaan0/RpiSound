#ifndef _IAUDIO_DEVICE_MANAGER_HPP__
#define _IAUDIO_DEVICE_MANAGER_HPP__

#include <string>
#include <vector>

#include "audio_utils.hpp"

class IAudioDeviceManager {
public:
    virtual std::vector<AudioDevice> listDevices() = 0;
    virtual bool setDevice(int32_t cardId, int32_t deviceId, AudioDevice::Type type) = 0;
    virtual void writeData(const std::vector<uint8_t>& data) = 0;
    virtual ~IAudioDeviceManager() = default;
};

#endif // _IAUDIO_DEVICE_MANAGER_HPP__
