#pragma once

#include "interfaces/iaudio_device_factory.hpp"
#include "audio_device.hpp"

class AudioDeviceFactory : public IAudioDeviceFactory {
public:
    Result<std::unique_ptr<IAudioDevice>> createAudioDevice(const types::AudioDeviceInfo& deviceInfo, IAudioDriver& audioDriver) override {
        return std::make_unique<AudioDevice>(audioDriver, deviceInfo);
    }
};
