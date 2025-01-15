#include <iostream>

#include "rpi_sound/audio_device_manager.hpp"
#include "rpi_sound/tiny_alsa_wrapper.hpp"
#include "rpi_sound/player.hpp"

extern "C" {
#include "tinyalsa/pcm.h"
}

bool Player::play(const std::string_view& filePath) {
    if (!converter_->load(filePath)) {
        std::cout << "Loading failed!\r\n";
        return false;
    }

    std::vector<std::tuple<uint32_t, uint32_t>> availableDevices;

    audio_device_ = std::make_unique<AudioDeviceManager>(std::make_unique<TinyAlsaWrapper>());
    auto devices = audio_device_->listDevices();
    for (auto& audioDevice : devices) {
        std::cout << "\r\nCard: " << audioDevice.card << ", Driver: " << audioDevice.driver << "\n";
        for (const auto& dev : audioDevice.device) {
            const auto& format = std::get<2>(dev);
            const auto type = std::get<1>(dev);
            if (type == AudioDevice::Type::kPlayback) {
                availableDevices.emplace_back(audioDevice.card, std::get<0>(dev));
            }
            std::cout << "  Device: " << std::get<0>(dev)
                      << ", Type: " << std::get<1>(dev)
                      << ", Sample Rate: " << format.audioFormat.sampleRate
                      << ", Channels: " << format.audioFormat.channels << "\n";
        }
    }
    for (auto& playBackDevice : availableDevices) {
        if (!audio_device_->setDevice(std::get<0>(playBackDevice),
                                      std::get<1>(playBackDevice),
                                      AudioDevice::Type::kPlayback)) {
            std::cout << "Failed to play on: Card " << std::get<0>(playBackDevice) <<
                         " Device " << std::get<1>(playBackDevice) << "\r\n";
            continue;
        }
        std::cout << "Playing on: Card " << std::get<0>(playBackDevice) <<
                     " Device " << std::get<1>(playBackDevice) << "\r\n";
        audio_device_->writeData(converter_->getData()->data);
    }
    
    return true;
}
