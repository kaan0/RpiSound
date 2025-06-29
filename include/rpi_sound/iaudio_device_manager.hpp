#pragma once

#include "alsa_driver.hpp"
#include "iaudio_device.hpp"
#include "types/audio_device_info.hpp"

class IAudioDeviceManager {
public:
    // Initialize the manager with dependencies
    virtual void initialize(std::unique_ptr<AlsaDriver> alsaDriver) = 0;

    // Check if the manager is initialized
    virtual bool isInitialized() const = 0;

    // Get a list of available audio devices
    virtual std::vector<types::AudioDeviceInfo> getAvailableDevices() const = 0;

    // Get the current audio device
    virtual types::AudioDeviceInfo getDevice() const = 0;

    // Open an audio device for playback or capture
    virtual bool openDevice(const types::AudioDeviceInfo& deviceInfo) = 0;

    // Close the currently opened audio device
    virtual void closeDevice() = 0;

    // Check if an audio device is currently open
    virtual bool isDeviceOpen() const = 0;

    // Get the currently opened audio device
    virtual std::shared_ptr<IAudioDevice> getCurrentDevice() const = 0;

    // Destructor
    virtual ~IAudioDeviceManager() = default;
};