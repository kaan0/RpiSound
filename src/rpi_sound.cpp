#include <iostream>
#include <memory>

#include <spdlog/spdlog.h>

#include "alsa/alsa_driver.hpp"
#include "alsa/alsa_device_enumerator.hpp"
#include "rpi_sound/audio_device_manager.hpp"
#include "rpi_sound/audio_device_factory.hpp"
#include "rpi_sound/pcm_loader.hpp"
#include "rpi_sound/sound_manager.hpp"

int main() {
    spdlog::info("Starting Raspberry Pi Sound System…");

    // Initialize the ALSA driver
    AlsaDriver alsaDriver;
    AlsaDeviceEnumerator alsaEnumerator;
    AudioDeviceFactory deviceFactory;

    // Initialize the audio device manager
    auto audioDeviceManager = AudioDeviceManager(deviceFactory, alsaEnumerator, alsaDriver);

    if (!audioDeviceManager.isInitialized()) {
        spdlog::error("Failed to initialize Audio Device Manager.");
        return -1;
    }

    SoundManager soundManager(std::make_unique<PcmLoader>(), audioDeviceManager);
    if (!soundManager.initialize()) {
        spdlog::error("Failed to initialize Sound Manager.");
        return -1;
    }

    auto availableDevices = soundManager.getAvailableAudioDevices();
    if (availableDevices.empty()) {
        spdlog::error("No audio devices available.");
        return -1;
    }

    // Test available devices
    for (const auto& device : availableDevices) {
        if (!soundManager.selectAudioDevice(device)) {
            spdlog::error("Failed to select audio device: {}", device.description);
        } else {
            spdlog::info(
                "Selected audio device: {} (Card: {}, Device: {})", device.description, device.cardId, device.deviceId);
        }
    }

    for (const auto& device : availableDevices) {
        if ((device.description.find("Jabra EVOLVE LINK MS") == std::string::npos) || (device.type != types::AudioDeviceInfo::kPlayback)) {
            continue;
        }

        if (!soundManager.selectAudioDevice(device)) {
            spdlog::error("Failed to select audio device: {}", device.description);
        }
        break;
    }

    // Trigger a sound sample
    if (!soundManager.triggerSound("kick", 100)) {
        spdlog::error("Failed to trigger sound sample.");
        return -1;
    }

    return 0;
}
