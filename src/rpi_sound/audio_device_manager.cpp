#include <fstream>
#include <iostream>
#include <regex>

#include "rpi_sound/audio_device.hpp"
#include "rpi_sound/audio_device_manager.hpp"
#include "utilities/logger.hpp"

AudioDeviceManager& AudioDeviceManager::getInstance() {
    static AudioDeviceManager instance;
    return instance;
}

void AudioDeviceManager::initialize(std::unique_ptr<AlsaDriver> alsaDriver) {
    if (m_initialized) {
        utilities::log.warning("AudioDeviceManager is already initialized.");
    }
    m_alsaDriver = std::move(alsaDriver);
    m_initialized = true;

    std::vector<types::AudioDeviceInfo> devices;

    // Read the ALSA devices file
    std::ifstream devicesFile{kDevicesPath};
    if (!devicesFile.good()) {
        utilities::log.error("Failed to open ALSA devices file: {}", kDevicesPath);
        return;
    }

    if (!parseDevicesFile(devicesFile, devices)) {
        utilities::log.error("Failed to parse ALSA devices file.");
        return;
    }

    // Read the ALSA cards file
    std::ifstream cardsFile{kCardsPath};
    if (!cardsFile.good()) {
        utilities::log.error("Failed to open ALSA cards file: {}", kCardsPath);
        return;
    }

    if (!parseCardsFile(cardsFile, devices)) {
        utilities::log.error("Failed to parse ALSA cards file.");
        return;
    }

    for (auto it = devices.begin(); it != devices.end();) {
        types::AudioDeviceInfo::DeviceFormat format;
        if (getDeviceFormat(it->cardId, it->deviceId, it->type, format)) {
            it->format = format;
            utilities::log.info("Device found: {} (Card: {}, Device: {})", it->description, it->cardId, it->deviceId);
            utilities::log.info(
                "Format: {} Hz, {}, Channels: {}, Period Size: {}, Period Count: {}, Type: {}",
                format.sampleRate,
                (format.sampleFormat == types::AudioDeviceInfo::DeviceFormat::kFormatS16LE   ? "S16LE"
                 : format.sampleFormat == types::AudioDeviceInfo::DeviceFormat::kFormatS32LE ? "S32LE"
                 : format.sampleFormat == types::AudioDeviceInfo::DeviceFormat::kFormatFloat ? "FLOAT"
                                                                                             : "UNKNOWN"),
                format.channelCount,
                format.periodSize,
                format.periodCount,
                (it->type == types::AudioDeviceInfo::DeviceType::kPlayback  ? "Playback"
                 : it->type == types::AudioDeviceInfo::DeviceType::kCapture ? "Capture"
                                                                            : "Invalid"));
            ++it;
        } else {
            utilities::log.warning(
                "Failed to get device format for card {}, device {}. Removing device.", it->cardId, it->deviceId);
            it = devices.erase(it);
        }
    }

    if (!devices.empty()) {
        m_availableDevices = std::move(devices);
    } else {
        utilities::log.error("No audio devices found in ALSA cards or devices files.");
    }
}

bool AudioDeviceManager::isInitialized() const {
    return m_initialized && m_alsaDriver != nullptr;
}

std::vector<types::AudioDeviceInfo> AudioDeviceManager::getAvailableDevices() const {
    return m_availableDevices;
}

types::AudioDeviceInfo AudioDeviceManager::getDevice() const {
    if (m_currentDevice) {
        return m_currentDevice->getDeviceInfo();
    }
    return types::AudioDeviceInfo{};
}

bool AudioDeviceManager::openDevice(const types::AudioDeviceInfo& deviceInfo) {
    if (!m_initialized || !m_alsaDriver) {
        utilities::log.error("AudioDeviceManager is not initialized.");
        return false;
    }

    if (m_currentDevice && m_currentDevice->isOpen()) {
        utilities::log.warning("An audio device is already open. Closing the current device.");
    }

    m_currentDevice = std::make_unique<AudioDevice>(m_alsaDriver);
    if (!m_currentDevice->open(deviceInfo)) {
        utilities::log.error("Failed to open audio device: {}", m_currentDevice->getLastError());
        return false;
    }
    return true;
}

void AudioDeviceManager::closeDevice() {
    if (m_currentDevice && m_currentDevice->isOpen()) {
        m_currentDevice->close();
    } else {
        utilities::log.warning("No audio device is currently open. Cannot close.");
    }
}

bool AudioDeviceManager::isDeviceOpen() const {
    return m_currentDevice && m_currentDevice->isOpen();
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
bool AudioDeviceManager::parseCardsFile(std::istream& cardsFile, std::vector<types::AudioDeviceInfo>& devices) const {
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
        return false;
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

    return true;
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
bool AudioDeviceManager::parseDevicesFile(std::istream& devicesFile,
                                          std::vector<types::AudioDeviceInfo>& devices) const {
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

    return isDeviceFound;
}

const types::AudioDeviceInfo::DeviceFormat& AudioDeviceManager::getDefaultDeviceFormat() const {
    static types::AudioDeviceInfo::DeviceFormat defaultFormat{
        .periodSize = 1024,
        .periodCount = 2,
        .startTreshold = 1024,     // periodSize
        .stopTreshold = 1024 * 2,  // periodSize * periodCount
        .silenceTreshold = 0,
        .silenceSize = 0,
        .channelCount = 2,                                                  // Stereo
        .sampleRate = 44100,                                                // Common sample rate
        .sampleFormat = types::AudioDeviceInfo::DeviceFormat::kFormatS32LE  // 16-bit signed little-endian
    };
    return defaultFormat;
}

bool AudioDeviceManager::getDeviceFormat(int32_t cardId,
                                         int32_t deviceId,
                                         types::AudioDeviceInfo::DeviceType type,
                                         types::AudioDeviceInfo::DeviceFormat& format) {
    auto params = m_alsaDriver->pcmParamsGet(cardId, deviceId, AudioDevice::toAlsaFlag(type));
    if (!params) {
        return false;
    }

    auto sampleFormat = types::AudioDeviceInfo::DeviceFormat::kFormatInvalid;
    std::vector<std::pair<AlsaDriver::PcmFormat, const char*>> pcmFormats = {
        {AlsaDriver::kFormatS16LE, "16-bit signed little-endian"},
        {AlsaDriver::kFormatS32LE, "32-bit signed little-endian"}};

    for (const auto& [pcmTestFormat, formatStr] : pcmFormats) {
        auto testResult = m_alsaDriver->pcmTestFormat(params, pcmTestFormat);
        if (testResult <= 0) {
            utilities::log.warning(
                "PCM format {} is not supported for card {}, device {}", formatStr, cardId, deviceId);
            continue;
        }
        sampleFormat = static_cast<types::AudioDeviceInfo::DeviceFormat::SampleFormat>(pcmTestFormat);
        break;  // Exit loop after finding a valid format
    }

    const auto defaultFormat = getDefaultDeviceFormat();
    format = {
        .periodSize =
            std::max(m_alsaDriver->pcmParamsGetMin(params, AlsaDriver::kParamPeriodSize), defaultFormat.periodSize),
        .periodCount =
            std::max(m_alsaDriver->pcmParamsGetMin(params, AlsaDriver::kParamPeriodCount), defaultFormat.periodCount),
        .startTreshold = defaultFormat.startTreshold,
        .stopTreshold = defaultFormat.stopTreshold,
        .silenceTreshold = defaultFormat.silenceTreshold,
        .silenceSize = defaultFormat.silenceSize,
        .channelCount =
            std::min(m_alsaDriver->pcmParamsGetMax(params, AlsaDriver::kParamChannels), defaultFormat.channelCount),
        .sampleRate = std::min(m_alsaDriver->pcmParamsGetMax(params, AlsaDriver::kParamRate), defaultFormat.sampleRate),
        .sampleFormat = sampleFormat};
    m_alsaDriver->pcmParamsFree(params);
    return true;
}
