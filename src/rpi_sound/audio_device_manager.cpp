#include <fstream>
#include <iostream>
#include <regex>

#include <spdlog/spdlog.h>

#include "rpi_sound/audio_device.hpp"
#include "rpi_sound/audio_device_manager.hpp"

AudioDeviceManager::AudioDeviceManager(IAudioDeviceFactory& deviceFactory,
                                       IDeviceEnumerator& deviceEnumerator,
                                       IAudioDriver& audioDriver)
    : m_deviceFactory{deviceFactory}, m_deviceEnumerator{deviceEnumerator}, m_audioDriver{audioDriver} {

    if (auto r = m_deviceEnumerator.list(types::AudioDeviceInfo::kPlayback)) {
        m_playbackDevices = std::move(r.value());
    } else {
        spdlog::warn("No playback devices found: {}", r.error());
    }

    if (auto r = m_deviceEnumerator.list(types::AudioDeviceInfo::kCapture)) {
        m_captureDevices = std::move(r.value());
    } else {
        spdlog::warn("No capture devices found: {}", r.error());
    }

    spdlog::info("Found {} playback and {} capture devices.", m_playbackDevices.size(), m_captureDevices.size());
}

bool AudioDeviceManager::isInitialized() const {
    return !(m_captureDevices.empty() && m_playbackDevices.empty());
}

Result<std::vector<types::AudioDeviceInfo>> AudioDeviceManager::getAvailableDevices(
    types::AudioDeviceInfo::DeviceType type) const {
    if (m_captureDevices.empty() && m_playbackDevices.empty()) {
        return std::unexpected("No devices available.");
    }

    std::vector<types::AudioDeviceInfo> devices;
    devices.reserve(m_captureDevices.size() + m_playbackDevices.size());

    auto append = [&devices](const std::vector<types::AudioDeviceInfo>& src) {
        devices.insert(devices.end(), src.begin(), src.end());
    };

    switch (type) {
        case types::AudioDeviceInfo::kPlayback:
            append(m_playbackDevices);
            break;
        case types::AudioDeviceInfo::kCapture:
            append(m_captureDevices);
            break;
        case types::AudioDeviceInfo::kAll:
            append(m_captureDevices);
            append(m_playbackDevices);
            break;
        default:
            return std::unexpected("Invalid device type requested.");
    }

    if (devices.empty()) {
        return std::unexpected("No devices available for the requested type: " +
                               types::AudioDeviceInfo::to_string(type));
    }

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
        spdlog::warn("An audio device is already open. Closing the current device.");
    }

    auto device_result = m_deviceFactory.createAudioDevice(deviceInfo, m_audioDriver);

    if (!device_result) {
        return std::unexpected("Could not create device. Error: " + device_result.error());
    }

    m_currentDevice = std::move(device_result.value());

    return m_currentDevice->open();
}

void AudioDeviceManager::closeDevice() noexcept {
    if (isDeviceOpen()) {
        m_currentDevice->close();
    } else {
        spdlog::warn("No audio device is currently open. Cannot close.");
    }
}

bool AudioDeviceManager::isDeviceOpen() const {
    if (!m_currentDevice) return false;
    auto r = m_currentDevice->isOpen();
    return r.has_value() && r.value();
}
