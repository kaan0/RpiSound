#pragma once

#include <string_view>

#include "types/audio_device_info.hpp"
#include "types/result.hpp"

using types::Result;

class IDeviceEnumerator {
public:
    virtual ~IDeviceEnumerator() = default;

    virtual Result<std::vector<types::AudioDeviceInfo>> list(types::AudioDeviceInfo::DeviceType type) = 0;
};
