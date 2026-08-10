#pragma once

#include "interfaces/idevice_enumerator.hpp"

using types::Result;

class AlsaDeviceEnumerator final : public IDeviceEnumerator {
public:
    Result<std::vector<types::AudioDeviceInfo>> list(types::AudioDeviceInfo::DeviceType type) override;

private:
    static constexpr std::string_view kCardsPath{"/proc/asound/cards"};      // Path to the ALSA cards file
    static constexpr std::string_view kDevicesPath{"/proc/asound/devices"};  // Path to the ALSA devices file

    Result<void> parseCardsFile(std::istream& cardsFile, std::vector<types::AudioDeviceInfo>& devices) const;
    Result<void> parseDevicesFile(std::istream& devicesFile, std::vector<types::AudioDeviceInfo>& devices) const;
    Result<types::AudioDeviceInfo::DeviceFormat> getDeviceFormat(const types::AudioDeviceInfo& deviceInfo) const;
};
