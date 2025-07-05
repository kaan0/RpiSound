#include <iostream>

#include "rpi_sound/sound_manager.hpp"
#include "utilities/logger.hpp"

bool SoundManager::initialize() {
    if (!m_audioDeviceManager.isInitialized()) {
        return false;  // Audio device manager is not initialized
    }
    if (!m_soundLoader->load(SoundManager::kDefaultInstrumentType)) {
        return false;  // Failed to load default sound samples
    }
    return true;  // Successfully initialized sound manager
}

std::vector<types::AudioDeviceInfo> SoundManager::getAvailableAudioDevices() const {
    if (!m_audioDeviceManager.isInitialized()) {
        return {};  // Audio device manager is not initialized
    }
    return m_audioDeviceManager.getAvailableDevices();  // Return the list of available audio devices
}

bool SoundManager::selectAudioDevice(const types::AudioDeviceInfo& deviceInfo) {
    if (!m_audioDeviceManager.isInitialized()) {
        return false;  // Audio device manager is not initialized
    }
    utilities::log.info("Selecting audio device: {} (Card: {}, Device: {}, Type: {})",
                        deviceInfo.description,
                        deviceInfo.cardId,
                        deviceInfo.deviceId,
                        types::AudioDeviceInfo::to_string(deviceInfo.type));
    return m_audioDeviceManager.openDevice(deviceInfo);  // Attempt to open the specified audio device
}

bool SoundManager::load(const std::string_view instrumentType) {
    if (!m_soundLoader->load(std::string(instrumentType))) {
        return false;  // Failed to load sound samples
    }
    return true;  // Successfully loaded sound samples
}

bool SoundManager::triggerSound(const std::string_view sampleName, uint32_t velocity) {
    if (!m_soundLoader->getSample(sampleName).audioData.empty()) {
        // Trigger the sound sample with the specified name and velocity
        auto& sample = m_soundLoader->getSample(sampleName);

        utilities::log.info("Triggering sound: {} with velocity: {}", sampleName, velocity);

        m_audioDeviceManager.getCurrentDevice()->write(sample.getAudioSpan());  // Write the audio data to the device
        return true;                                                            // Successfully triggered sound
    }
    return false;  // Sound sample not found
}