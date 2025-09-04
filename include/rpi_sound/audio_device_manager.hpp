#pragma once

#include <memory>

#include "interfaces/iaudio_device.hpp"
#include "interfaces/iaudio_driver.hpp"
#include "interfaces/iaudio_device_manager.hpp"

using namespace std::literals;

class AudioDeviceManager : public IAudioDeviceManager {
public:
    static constexpr std::string_view kCardsPath = "/proc/asound/cards"sv;      // Path to the ALSA cards file
    static constexpr std::string_view kDevicesPath = "/proc/asound/devices"sv;  // Path to the ALSA devices file

    // Constructor
    AudioDeviceManager(IAudioDeviceFactory& deviceFactory,
                    IDeviceEnumerator& deviceEnumerator,
                    IAudioDriver& audioDriver);

    // Destructor
    ~AudioDeviceManager() override = default;

    // Delete copy constructor and assignment operator
    AudioDeviceManager(const AudioDeviceManager&) = delete;
    AudioDeviceManager& operator=(const AudioDeviceManager&) = delete;

    // Check if the manager is initialized
    bool isInitialized() const override;

    // Get a list of available audio devices
    Result<std::vector<types::AudioDeviceInfo>> getAvailableDevices() const override;

    // Get the current audio device
    Result<std::shared_ptr<IAudioDevice>> getDevice() const override;

    // Open an audio device for playback or capture
    Result<void> openDevice(const types::AudioDeviceInfo& deviceInfo) override;

    // Close the currently opened audio device
    void closeDevice() noexcept override;

    // Check if an audio device is currently open
    bool isDeviceOpen() const override;

private:

    // Pointer to the currently opened audio device
    std::shared_ptr<IAudioDevice> m_currentDevice;

    std::vector<types::AudioDeviceInfo> m_playbackDevices;

    std::vector<types::AudioDeviceInfo> m_captureDevices;

    // Initialization flag
    bool m_initialized = false;

    IAudioDeviceFactory& m_deviceFactory;
    IDeviceEnumerator& m_deviceEnumerator;
    IAudioDriver& m_audioDriver;
};
