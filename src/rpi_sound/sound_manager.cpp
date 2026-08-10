#include <iostream>

#include <spdlog/spdlog.h>

#include "rpi_sound/sound_manager.hpp"

bool SoundManager::initialize() {
    if (!m_audioDeviceManager.isInitialized()) {
        return false;  // Audio device manager is not initialized
    }
    if (!m_soundLoader->load(SoundManager::kDefaultInstrumentType)) {
        return false;  // Failed to load default sound samples
    }
    return true;  // Successfully initialized sound manager
}

std::vector<types::AudioDeviceInfo> SoundManager::getAvailableAudioDevices(
    types::AudioDeviceInfo::DeviceType type) const {
    if (!m_audioDeviceManager.isInitialized()) {
        return {};  // Audio device manager is not initialized
    }
    auto devices_result = m_audioDeviceManager.getAvailableDevices(type);
    if (!devices_result) {
        return {};
    }

    return devices_result.value();  // Return the list of available audio devices
}

std::vector<std::string> SoundManager::getAvailableAudioDeviceDescriptions(
    types::AudioDeviceInfo::DeviceType type) const {
    std::vector<types::AudioDeviceInfo> devices = getAvailableAudioDevices(type);

    std::vector<std::string> deviceDescriptions;
    for (const auto& device : devices) {
        if (device.type == type || type == types::AudioDeviceInfo::kAll) {
            deviceDescriptions.push_back(device.description);
        }
    }
    return deviceDescriptions;  // Return the list of available audio device descriptions
}

bool SoundManager::selectAudioDevice(const types::AudioDeviceInfo& deviceInfo) {
    if (!m_audioDeviceManager.isInitialized()) {
        return false;  // Audio device manager is not initialized
    }
    spdlog::info("Selecting audio device: {} (Card: {}, Device: {}, Type: {})",
                 deviceInfo.description,
                 deviceInfo.cardId,
                 deviceInfo.deviceId,
                 types::AudioDeviceInfo::to_string(deviceInfo.type));
    auto open_device_result = m_audioDeviceManager.openDevice(deviceInfo);
    if (!open_device_result) {
        spdlog::error("Opening device failed: {}", open_device_result.error());
        return false;
    }
    auto audio_device = m_audioDeviceManager.getDevice();
    if (!audio_device) {
        spdlog::error("Device error: {}", audio_device.error());
    }
    if (m_audioEngine->isRunning()) {
        m_audioEngine->stop();
    }
    m_audioEngine->start(std::move(audio_device.value()));
    return true;
}

bool SoundManager::load(const std::string_view instrumentType) {
    if (!m_soundLoader->load(instrumentType)) {
        return false;  // Failed to load sound samples
    }
    return true;  // Successfully loaded sound samples
}

bool SoundManager::triggerSound(const std::string_view sampleName, uint32_t velocity) {
    auto sample = m_soundLoader->getSample(sampleName);
    if (!sample || sample->audioData.empty()) {
        return false;  // Sample not found or has no data
    }

    const float gain = static_cast<float>(velocity) / 127.0f;
    spdlog::info("Triggering sound: {} with velocity: {}", sampleName, velocity);

    m_audioEngine->writeSample(std::move(sample), gain);
    return true;
}
