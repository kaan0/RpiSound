#pragma once

#include "interfaces/idevice_enumerator.hpp"

using types::Result;

class AlsaDeviceEnumerator final : public IDeviceEnumerator {
public:
    Result<std::vector<types::AudioDeviceInfo>> list(types::AudioDeviceInfo::DeviceType type,
    std::string_view cards_file_path, std::string_view devices_file_path) override;

private:
    Result<void> parseCardsFile(std::istream& cardsFile, std::vector<types::AudioDeviceInfo>& devices) const;
    Result<void> parseDevicesFile(std::istream& devicesFile, std::vector<types::AudioDeviceInfo>& devices) const;
    Result<types::AudioDeviceInfo::DeviceFormat> getDeviceFormat(const types::AudioDeviceInfo& deviceInfo) const;
};
