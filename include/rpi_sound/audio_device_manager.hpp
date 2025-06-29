#pragma once

#include <memory>

#include "alsa_driver.hpp"
#include "iaudio_device.hpp"
#include "iaudio_device_manager.hpp"

class AudioDeviceManager : public IAudioDeviceManager {
public:
    static constexpr const char* kCardsPath = "/proc/asound/cards";      // Path to the ALSA cards file
    static constexpr const char* kDevicesPath = "/proc/asound/devices";  // Path to the ALSA devices file

    // Singleton instance retrieval
    static AudioDeviceManager& getInstance();

    // Destructor
    ~AudioDeviceManager() override = default;

    // Delete copy constructor and assignment operator
    AudioDeviceManager(const AudioDeviceManager&) = delete;
    AudioDeviceManager& operator=(const AudioDeviceManager&) = delete;

    // Initialize the manager with dependencies
    void initialize(std::unique_ptr<AlsaDriver> alsaDriver) override;

    // Check if the manager is initialized
    bool isInitialized() const override;

    // Get a list of available audio devices
    std::vector<types::AudioDeviceInfo> getAvailableDevices() const override;

    // Get the current audio device
    types::AudioDeviceInfo getDevice() const override;

    // Open an audio device for playback or capture
    bool openDevice(const types::AudioDeviceInfo& deviceInfo) override;

    // Close the currently opened audio device
    void closeDevice() override;

    // Check if an audio device is currently open
    bool isDeviceOpen() const override;

    // Get the currently opened audio device
    std::shared_ptr<IAudioDevice> getCurrentDevice() const override { return m_currentDevice; }

private:
    // Private constructor for singleton pattern
    AudioDeviceManager() = default;

    // Helper function to parse the ALSA cards file
    bool parseCardsFile(std::istream& cardsFile, std::vector<types::AudioDeviceInfo>& devices) const;

    // Helper function to parse the ALSA devices file
    bool parseDevicesFile(std::istream& devicesFile, std::vector<types::AudioDeviceInfo>& devices) const;

    // Get the default device format
    const types::AudioDeviceInfo::DeviceFormat& getDefaultDeviceFormat() const;

    // Get the device format for a specific card and device
    bool getDeviceFormat(int32_t cardId,
                         int32_t deviceId,
                         types::AudioDeviceInfo::DeviceType type,
                         types::AudioDeviceInfo::DeviceFormat& format);

    // Pointer to the currently opened audio device
    std::shared_ptr<IAudioDevice> m_currentDevice;

    // List of available audio devices
    std::vector<types::AudioDeviceInfo> m_availableDevices;

    // Pointer to the ALSA driver instance
    std::shared_ptr<AlsaDriver> m_alsaDriver;

    // Initialization flag
    bool m_initialized = false;
};
