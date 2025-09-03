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
    void initialize(IAudioDeviceFactory& deviceFactory,
                    IDeviceEnumerator& deviceEnumerator,
                    IAudioDriver& audioDriver) override;

    // Check if the manager is initialized
    bool isInitialized() const override;

    // Get a list of available audio devices
    Result<std::vector<types::AudioDeviceInfo>> getAvailableDevices() const override;

    // Get the current audio device
    Result<types::AudioDeviceInfo> getDevice() const override;

    // Open an audio device for playback or capture
    Result<void> openDevice(const types::AudioDeviceInfo& deviceInfo) override;

    // Close the currently opened audio device
    void closeDevice() noexcept override;

    // Check if an audio device is currently open
    bool isDeviceOpen() const override;

    // Get the currently opened audio device
    Result<std::shared_ptr<IAudioDevice>> getDevice(const types::AudioDeviceInfo& deviceInfo) const override;

private:
    // Private constructor for singleton pattern
    AudioDeviceManager() = default;

    // Pointer to the currently opened audio device
    std::shared_ptr<IAudioDevice> m_currentDevice;

    // List of available audio devices
    std::vector<types::AudioDeviceInfo> m_availableDevices;

    // Initialization flag
    bool m_initialized = false;

    IAudioDeviceFactory& m_deviceFactory;
    IDeviceEnumerator& m_deviceEnumerator;
    IAudioDriver& m_audioDriver;
};
