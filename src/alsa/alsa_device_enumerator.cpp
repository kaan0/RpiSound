#include <fstream>
#include <regex>

#include "alsa/alsa_device_enumerator.hpp"
#include "alsa/alsa_utils.hpp"
#include "utilities/logger.hpp"

Result<std::vector<types::AudioDeviceInfo>> AlsaDeviceEnumerator::list(types::AudioDeviceInfo::DeviceType type,
    std::string_view cards_file_path, std::string_view devices_file_path) {

    // TODO: Fix unnecessary copy
    auto devices_file_path_str{std::string(devices_file_path)};
    std::ifstream devicesFile{devices_file_path_str};
    if (!devicesFile.good()) {
        return std::unexpected("Failed to open ALSA devices file: " + devices_file_path_str);
    }

    // TODO: Fix unnecessary copy
    auto cards_file_path_str{std::string(cards_file_path)};
    std::ifstream cardsFile{cards_file_path_str};
    if (!cardsFile.good()) {
        return std::unexpected("Failed to open ALSA cards file: " + cards_file_path_str);
    }

    std::vector<types::AudioDeviceInfo> devices;

    // Parse the ALSA devices file
    if (auto result = parseDevicesFile(devicesFile, devices); !result) {
        return std::unexpected("Failed to parse ALSA devices file: " + devices_file_path_str);
    }

    // Parse the ALSA cards file
    if (auto result = parseCardsFile(cardsFile, devices); !result) {
        return std::unexpected("Failed to parse ALSA cards file: " + cards_file_path_str);
    }

    for (auto it = devices.begin(); it != devices.end();) {
        if (auto result = getDeviceFormat(*it); !result) {
            utilities::log.warning(
                "Failed to get device format for card {}, device {}. Removing device.", it->cardId, it->deviceId);
            it = devices.erase(it);
        // TODO: fix type selection here
        } else {
            it->format = result.value();
            utilities::log.info("Device found: {} (Card: {}, Device: {})", it->description, it->cardId, it->deviceId);
            utilities::log.info(
                "Format: {} Hz, {}, Channels: {}, Period Size: {}, Period Count: {}, Type: {}",
                it->format.sampleRate,
                (it->format.sampleFormat == types::AudioDeviceInfo::DeviceFormat::kFormatS16LE   ? "S16LE"
                 : it->format.sampleFormat == types::AudioDeviceInfo::DeviceFormat::kFormatS32LE ? "S32LE"
                 : it->format.sampleFormat == types::AudioDeviceInfo::DeviceFormat::kFormatFloat ? "FLOAT"
                                                                                                 : "UNKNOWN"),
                it->format.channelCount,
                it->format.periodSize,
                it->format.periodCount,
                (it->type == types::AudioDeviceInfo::DeviceType::kPlayback  ? "Playback"
                 : it->type == types::AudioDeviceInfo::DeviceType::kCapture ? "Capture"
                                                                            : "Invalid"));
            ++it;
        }
    }

    if (devices.empty()) {
        return std::unexpected("No valid devices found");
    }

    return devices;
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
Result<void> AlsaDeviceEnumerator::parseCardsFile(std::istream& cardsFile, std::vector<types::AudioDeviceInfo>& devices) const {
    std::regex lineRegex(R"(^\s(\d+)\s\[(\S+)\s*\]:\s+(\S+)\s+-\s+(.+)$)");
    std::regex longnameRegex(R"(^\s+(.+)$)");

    std::string line;
    auto isDeviceFound{false};
    std::vector<types::AudioDeviceInfo> deviceInfos;

    while (std::getline(cardsFile, line)) {
        std::smatch match;
        types::AudioDeviceInfo deviceInfo;
        if (std::regex_match(line, match, lineRegex)) {
            deviceInfo.cardId = std::atoi(match[1].str().c_str());
            deviceInfo.driver = match[2].str();
            isDeviceFound = true;
            deviceInfos.push_back(deviceInfo);
        } else if (std::regex_match(line, match, longnameRegex)) {
            deviceInfo.description = match[1].str();
            if (!deviceInfos.empty()) {
                // Update the last deviceInfo with the description
                deviceInfos.back().description = deviceInfo.description;
            } else {
                utilities::log.error("No device info found for description: {}", deviceInfo.description);
            }
        }
    }

    if (!isDeviceFound) {
        return std::unexpected("No devices found in cards file");
    }
    // Merge deviceInfos into devices
    for (auto& device : devices) {
        for (const auto& info : deviceInfos) {
            if (device.cardId == info.cardId) {
                device.driver = info.driver;
                device.description = info.description;
                break;  // Found the matching card, no need to continue
            }
        }
    }
}


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

Result<void> AlsaDeviceEnumerator::parseDevicesFile(std::istream& devicesFile, std::vector<types::AudioDeviceInfo>& devices) const {
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
            auto cardId{std::atoi(match[1].str().c_str())};
            auto type{types::AudioDeviceInfo::DeviceType::kInvalid};
            if (match[3].str() == playbackId) {
                type = types::AudioDeviceInfo::DeviceType::kPlayback;
            } else if (match[3].str() == captureId) {
                type = types::AudioDeviceInfo::DeviceType::kCapture;
            } else {
                utilities::log.warning("Unknown device type: {}", match[3].str());
                continue;  // Skip unknown device types
            }
            types::AudioDeviceInfo deviceInfo;
            deviceInfo.cardId = cardId;
            deviceInfo.type = type;
            deviceInfo.deviceId = std::atoi(match[2].str().c_str());
            devices.push_back(deviceInfo);
            isDeviceFound = true;
        }
    }

    if (!isDeviceFound) {
        return std::unexpected("No devices found in devices file");
    }
}

Result<types::AudioDeviceInfo::DeviceFormat> AlsaDeviceEnumerator::getDeviceFormat(const types::AudioDeviceInfo& deviceInfo) const {
    auto params = AlsaFacade::pcmParamsGet(deviceInfo.cardId, deviceInfo.deviceId, alsa_utils::toAlsaFlag(deviceInfo.type));
    if (!params) {
        return std::unexpected("Failed to get PCM parameters");
    }

    auto sampleFormat = types::AudioDeviceInfo::DeviceFormat::kFormatInvalid;
    std::vector<std::pair<AlsaFacade::PcmFormat, const char*>> formatMap = {
        {AlsaFacade::kFormatS16LE, "16-bit signed little-endian"},
        {AlsaFacade::kFormatS32LE, "32-bit signed little-endian"}
    };

    for (const auto& [pcmTestFormat, formatStr] : formatMap) {
        auto testResult = AlsaFacade::pcmTestFormat(params, pcmTestFormat);
        if (testResult <= 0) {
            utilities::log.warning(
                "PCM format {} is not supported for card {}, device {}", formatStr, deviceInfo.cardId, deviceInfo.deviceId);
            continue;
        }
        sampleFormat = static_cast<types::AudioDeviceInfo::DeviceFormat::SampleFormat>(pcmTestFormat);
        break;  // Exit loop after finding a valid format
    }

    const auto defaultFormat = alsa_utils::getDefaultDeviceFormat();
    types::AudioDeviceInfo::DeviceFormat format = {
        .periodSize =
            std::max(AlsaFacade::pcmParamsGetMin(params, AlsaFacade::kParamPeriodSize), defaultFormat.periodSize),
        .periodCount =
            std::max(AlsaFacade::pcmParamsGetMin(params, AlsaFacade::kParamPeriodCount), defaultFormat.periodCount),
        .startTreshold = defaultFormat.startTreshold,
        .stopTreshold = defaultFormat.stopTreshold,
        .silenceTreshold = defaultFormat.silenceTreshold,
        .silenceSize = defaultFormat.silenceSize,
        .channelCount =
            std::min(AlsaFacade::pcmParamsGetMax(params, AlsaFacade::kParamChannels), defaultFormat.channelCount),
        .sampleRate = std::min(AlsaFacade::pcmParamsGetMax(params, AlsaFacade::kParamRate), defaultFormat.sampleRate),
        .sampleFormat = sampleFormat};

    AlsaFacade::pcmParamsFree(params);

    return format;
}
