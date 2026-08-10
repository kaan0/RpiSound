#pragma once

#include "interfaces/iaudio_device_factory.hpp"
#include "interfaces/iaudio_driver.hpp"
#include "interfaces/idevice_enumerator.hpp"

#include "types/audio_device_info.hpp"

class IAudioDeviceManager {
public:
    // Check if the manager is initialized
    virtual bool isInitialized() const = 0;

    // Get a list of available audio devices
    virtual Result<std::vector<types::AudioDeviceInfo>> getAvailableDevices(
        types::AudioDeviceInfo::DeviceType type) const = 0;

    // Get the current audio device
    virtual Result<std::shared_ptr<IAudioDevice>> getDevice() const = 0;

    // Open an audio device for playback or capture
    virtual Result<void> openDevice(const types::AudioDeviceInfo& deviceInfo) = 0;

    // Close the currently opened audio device
    virtual void closeDevice() noexcept = 0;

    // Check if an audio device is currently open
    virtual bool isDeviceOpen() const = 0;

    // Destructor
    virtual ~IAudioDeviceManager() = default;
};