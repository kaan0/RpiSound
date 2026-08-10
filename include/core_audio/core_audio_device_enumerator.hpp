#pragma once

#include "interfaces/idevice_enumerator.hpp"

using types::Result;

class CoreAudioDeviceEnumerator final : public IDeviceEnumerator {
public:
    Result<std::vector<types::AudioDeviceInfo>> list(types::AudioDeviceInfo::DeviceType type) override;

private:
    Result<types::AudioDeviceInfo::DeviceFormat> getDeviceFormat(const types::AudioDeviceInfo& deviceInfo) const;
};
