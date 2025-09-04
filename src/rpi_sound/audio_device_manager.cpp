#include <fstream>
#include <iostream>
#include <regex>

#include "rpi_sound/audio_device.hpp"
#include "rpi_sound/audio_device_manager.hpp"
#include "utilities/logger.hpp"

AudioDeviceManager& AudioDeviceManager::getInstance() {
    static AudioDeviceManager instance;
    return instance;
}

void AudioDeviceManager::initialize(IAudioDeviceFactory& deviceFactory,
                                    IDeviceEnumerator& deviceEnumerator,
                                    IAudioDriver& audioDriver) {

    m_deviceFactory = deviceFactory;
    m_deviceEnumerator = deviceEnumerator;
    m_audioDriver = audioDriver;

    // TODO: move paths to outside
    auto playback_device_list_result = m_deviceEnumerator.list(types::AudioDeviceInfo::kPlayback, kCardsPath, kDevicesPath);
    if (!playback_device_list_result) {
        utilities::log.warning("No PlayBack devices found. Warning: {}", playback_device_list_result.error());
    }

    m_playbackDevices = std::move(playback_device_list_result.value());

    auto capture_device_list_result = m_deviceEnumerator.list(types::AudioDeviceInfo::kCapture, kCardsPath, kDevicesPath);
    if (!capture_device_list_result) {
        utilities::log.warning("No Capture devices found. Warning: {}", capture_device_list_result.error());
    }

    m_captureDevices = std::move(capture_device_list_result.value());

    utilities::log.info("Found {} playback and {} capture devices.", m_playbackDevices.size(), m_captureDevices.size());
}

bool AudioDeviceManager::isInitialized() const {
    return !(m_captureDevices.empty() && m_playbackDevices.empty());
}

Result<std::vector<types::AudioDeviceInfo>> AudioDeviceManager::getAvailableDevices() const {
    if (m_captureDevices.empty() && m_playbackDevices.empty()) {
        return std::unexpected("No devices available.");
    }

    std::vector<types::AudioDeviceInfo> devices;
    devices.reserve(m_captureDevices.size() + m_playbackDevices.size());
    devices.insert(devices.end(), m_captureDevices.begin(), m_captureDevices.end());
    devices.insert(devices.end(), m_playbackDevices.begin(), m_playbackDevices.end());

    return devices;
}

Result<std::shared_ptr<IAudioDevice>> AudioDeviceManager::getDevice() const {
    if (m_currentDevice) {
        return m_currentDevice;
    }
    return std::unexpected("No devices to get.");
}

Result<void> AudioDeviceManager::openDevice(const types::AudioDeviceInfo& deviceInfo) {
    if (isDeviceOpen()) {
        utilities::log.warning("An audio device is already open. Closing the current device.");
    }

    auto device_result = m_deviceFactory.createAudioDevice(deviceInfo, m_audioDriver);

    if (!device_result) {
        return std::unexpected("Could not create device. Error: " + device_result.error());
    }

    m_currentDevice = std::move(device_result.value());

    return {};
}

void AudioDeviceManager::closeDevice() noexcept {
    if (isDeviceOpen()) {
        m_currentDevice->close();
    } else {
        utilities::log.warning("No audio device is currently open. Cannot close.");
    }
}

bool AudioDeviceManager::isDeviceOpen() const {
    return m_currentDevice && m_currentDevice->isOpen();
}
