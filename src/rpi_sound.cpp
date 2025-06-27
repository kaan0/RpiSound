#include <iostream>
#include <memory>

#include "rpi_sound/alsa_driver.hpp"
#include "rpi_sound/audio_device_manager.hpp"

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

    // Get available devices
    auto devices = audioDeviceManager.getAvailableDevices();
    if (devices.empty()) {
        std::cout << "No audio devices found." << std::endl;
        return -1;
    }

    // Print available devices
    for (const auto& device : devices) {
        std::cout << "Found device: " << device.description << " (Card: " << device.cardId
                  << ", Device: " << device.deviceId << ")" << std::endl;
    }

    return 0;
}