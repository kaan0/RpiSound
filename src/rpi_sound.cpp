#include <iostream>
#include <memory>

#include "rpi_sound/alsa_driver.hpp"
#include "rpi_sound/audio_device_manager.hpp"
#include "rpi_sound/pcm_loader.hpp"
#include "rpi_sound/sound_manager.hpp"

int main() {
    std::cout << "RpiSound!" << std::endl;

    // Initialize the ALSA driver
    auto alsaDriver = std::make_unique<AlsaDriver>();

    // Initialize the audio device manager
    AudioDeviceManager& audioDeviceManager = AudioDeviceManager::getInstance();
    audioDeviceManager.initialize(std::move(alsaDriver));

    if (!audioDeviceManager.isInitialized()) {
        std::cerr << "Failed to initialize Audio Device Manager." << std::endl;
        return -1;
    }

    SoundManager soundManager(std::make_unique<PcmLoader>(), audioDeviceManager);
    if (!soundManager.initialize()) {
        std::cerr << "Failed to initialize Sound Manager." << std::endl;
        return -1;
    }

    auto availableDevices = soundManager.getAvailableAudioDevices();
    if (availableDevices.empty()) {
        std::cerr << "No audio devices available." << std::endl;
        return -1;
    }

    // Print available devices
    for (const auto& device : availableDevices) {
        std::cout << "Found device: " << device.description << " (Card: " << device.cardId
                  << ", Device: " << device.deviceId << ")" << std::endl;
    }

    // Select the first available device
    if (!soundManager.selectAudioDevice(availableDevices.front())) {
        std::cerr << "Failed to select audio device." << std::endl;
        return -1;
    }

    // Trigger a sound sample
    if (!soundManager.triggerSound("snare_0", 100)) {
        std::cerr << "Failed to trigger sound sample." << std::endl;
        return -1;
    }

    return 0;
}
