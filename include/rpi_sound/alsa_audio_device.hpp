#pragma once

#include <unordered_map>

#include "iaudio_device.hpp"
#include "types/audio_device.hpp"

class AlsaAudioDevice : public IAudioDevice {
public:
    bool open(std::string deviceName) override;
    bool close() override;
    bool write(std::span<const types::audio_t> samples) override;
    std::vector<std::string> getDevices() const override;

private:
    std::unordered_map<std::string, types::AudioDevice> m_devices;  // Map of device names to AudioDevice objects
    std::string m_currentDeviceName;                                // Name of the currently opened device
    bool m_isOpen = false;                                          // Flag to check if the device is open

    // Helper function to initialize ALSA library and devices
    void initializeDevices();
};
