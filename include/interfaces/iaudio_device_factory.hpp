#pragma once

#include "interfaces/iaudio_device.hpp"
#include "interfaces/iaudio_driver.hpp"

class IAudioDeviceFactory {
public:
    virtual ~IAudioDeviceFactory() = default;
    virtual Result<std::unique_ptr<IAudioDevice>> createAudioDevice(const types::AudioDeviceInfo& deviceInfo, IAudioDriver& audioDriver) = 0;
};
