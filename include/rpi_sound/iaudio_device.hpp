#pragma once

#include <span>
#include <string>
#include <vector>

#include "types/audio_device.hpp"
#include "types/sound_sample.hpp"

class IAudioDevice {
public:
    virtual bool open(std::string deviceName) = 0;
    virtual bool close() = 0;
    virtual bool write(std::span<const types::audio_t> samples) = 0;
    virtual std::vector<std::string> getDevices() const = 0;
    virtual ~IAudioDevice() = default;
};
