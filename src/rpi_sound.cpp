#include <iostream>
#include <memory>

#include "alsa/alsa_driver.hpp"
#include "alsa/alsa_device_enumerator.hpp"
#include "rpi_sound/audio_device_manager.hpp"
#include "rpi_sound/audio_device_factory.hpp"
#include "rpi_sound/pcm_loader.hpp"
#include "rpi_sound/sound_manager.hpp"
#include "utilities/logger.hpp"

int main() {
    logging::redirectStdStreams();

    logging::Logger::Config cfg = {.toStdOut = true,
                                   .toFile = false,
                                   .filename = "rpi_sound.log",
                                   .maxFileSize = 5 * 1024 * 1024,  // 5 MB
                                   .level = logging::Level::kDebug};
    auto& logger = logging::Logger::getInstance(cfg);
    utilities::log.info("Starting Raspberry Pi Sound System…");

    // Initialize the ALSA driver
    AlsaDriver alsaDriver;
    AlsaDeviceEnumerator alsaEnumerator;
    AudioDeviceFactory deviceFactory;

    // Initialize the audio device manager
    auto audioDeviceManager = AudioDeviceManager(deviceFactory, alsaEnumerator, alsaDriver);

    if (!audioDeviceManager.isInitialized()) {
        utilities::log.error("Failed to initialize Audio Device Manager.");
        return -1;
    }

    SoundManager soundManager(std::make_unique<PcmLoader>(), audioDeviceManager);
    if (!soundManager.initialize()) {
        utilities::log.error("Failed to initialize Sound Manager.");
        return -1;
    }

    auto availableDevices = soundManager.getAvailableAudioDevices();
    if (availableDevices.empty()) {
        utilities::log.error("No audio devices available.");
        return -1;
    }

    // Test available devices
    for (const auto& device : availableDevices) {
        if (!soundManager.selectAudioDevice(device)) {
            utilities::log.error("Failed to select audio device: {}", device.description);
        } else {
            utilities::log.info(
                "Selected audio device: {} (Card: {}, Device: {})", device.description, device.cardId, device.deviceId);
        }
    }

    // Trigger a sound sample
    // if (!soundManager.triggerSound("snare_0", 100)) {
    //     utilities::log.error("Failed to trigger sound sample.");
    //     return -1;
    // }

    return 0;
}
