#include <cstdlib>
#include <iostream>
#include <fstream>
#include <regex>

#include "audio_device_manager.hpp"

namespace {
    constexpr auto kCardsFile{"/proc/asound/cards"};
    constexpr auto kDevicesFile{"/proc/asound/devices"};
}

/* cards:
 0 [Headphones     ]: bcm2835_headpho - bcm2835 Headphones
                      bcm2835 Headphones
 1 [vc4hdmi0       ]: vc4-hdmi - vc4-hdmi-0
                      vc4-hdmi-0
 2 [vc4hdmi1       ]: vc4-hdmi - vc4-hdmi-1
                      vc4-hdmi-1
 3 [A4             ]: USB-Audio - AIR 192 4
                      M-Audio AIR 192 4 at usb-0000:01:00.0-1.2, high speed
*/

/* devices:
  2: [ 0- 0]: digital audio playback
  3: [ 0]   : control
  4: [ 1- 0]: digital audio playback
  5: [ 1]   : control
  6: [ 2- 0]: digital audio playback
  7: [ 2]   : control
  8: [ 3- 0]: digital audio playback
  9: [ 3- 0]: digital audio capture
 10: [ 3]   : control
 33:        : timer
*/

bool AudioDeviceManager::parseCards(std::istream& cardsFile, std::vector<AudioDevice>& devices) {
    std::regex lineRegex(R"(^\s(\d+)\s\[(\S+)\s*\]:\s+(\S+)\s+-\s+(.+)$)");
    std::regex longnameRegex(R"(^\s+(.+)$)");

    std::string line;
    AudioDevice device;
    auto isDeviceFound{false};

    while (std::getline(cardsFile, line)) {
        std::smatch match;
        if (std::regex_match(line, match, lineRegex)) {
            device.card = std::atoi(match[1].str().c_str());
            device.driver = match[3].str();
        } else if (std::regex_match(line, match, longnameRegex)) {
            device.description = match[1].str();
            devices.push_back(device);
            isDeviceFound = true;
        }
    }

    return isDeviceFound;
}

bool AudioDeviceManager::parseDevices(std::istream& devicesFile, std::vector<AudioDevice>& devices) {
    std::regex devicesRegex(R"(^\s+\d+:\s+\[\s*(.+)\-\s+(.+)\]\:\s+(.+)$)");
    std::string playbackId{"digital audio playback"};
    std::string captureId{"digital audio capture"};
    std::string line;

    auto isDeviceFound{false};
    // TODO: solve cross-compile error with stoi
    // TODO: rework match[magic_number]

    while (std::getline(devicesFile, line)) {
        std::smatch match;
        if (std::regex_match(line, match, devicesRegex) && match.size() > 3) {
            auto cardId = std::atoi(match[1].str().c_str());
            if (cardId < devices.size()) {
                auto type{AudioDevice::Type::kInvalid};
                if (match[3].str() == playbackId) {
                    type = AudioDevice::Type::kPlayback;
                } else if (match[3].str() == captureId) {
                    type = AudioDevice::Type::kCapture;
                } else {
                    std::cout << "Match: " << match[3].str() << "\r\n";
                }
                devices[cardId].device.emplace_back(std::atoi(match[2].str().c_str()), type, HWAudioFormat{});
                isDeviceFound = true;
            }
        }
    }

    return isDeviceFound;
}

std::vector<AudioDevice> AudioDeviceManager::listDevices() {
    std::vector<AudioDevice> devices;
    std::ifstream cardsFile{kCardsFile};
    if (!cardsFile) {
        std::cout << "Failed to parse " << kCardsFile << "\r\n";
        return {};
    }

    if (!parseCards(cardsFile, devices)) {
        std::cout << "Failed to parse " << kCardsFile << "\r\n";
        return {};
    }

    std::ifstream devicesFile{kDevicesFile};
    if (!devicesFile) {
        std::cout << "Failed to parse " << kDevicesFile << "\r\n";
        return {};
    }

    if (!parseDevices(devicesFile, devices)) {
        std::cout << "Failed to parse " << kDevicesFile << "\r\n";
        return {};
    }

    for (auto& device : devices) {
        for (auto& subDevice : device.device) {
            auto deviceId = std::get<0>(subDevice);
            auto isOutput = std::get<1>(subDevice) == AudioDevice::Type::kPlayback;
            auto& format = std::get<2>(subDevice);
            driver_->getDeviceFormat(device.card, deviceId, isOutput, format);
        }
    }

    devices_ = devices;
    return devices;
}

bool AudioDeviceManager::setDevice(int32_t cardId, int32_t deviceId, AudioDevice::Type type) {
    for (auto& device : devices_) {
        if (device.card == cardId) {
            for (auto& subDevice : device.device) {
                int32_t subDevId;
                AudioDevice::Type subDevType;
                HWAudioFormat subDevFormat;
                std::tie(subDevId, subDevType, subDevFormat) = subDevice;
                if (type == subDevType) {
                    return driver_->openDevice(device.card,
                                              subDevId, type == AudioDevice::Type::kPlayback,
                                              subDevFormat);
                }
            }
        }
    }

    // fallback to default
    auto defaultFormat = driver_->getDefaultFormat();
    return driver_->openDevice(cardId, deviceId, type == AudioDevice::Type::kPlayback, defaultFormat);
}

void AudioDeviceManager::writeData(const std::vector<uint8_t>& data) {
    driver_->writeData(data);
}