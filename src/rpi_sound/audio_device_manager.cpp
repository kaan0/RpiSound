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

    for (auto& device : devices) {
        types::AudioDeviceInfo::DeviceFormat format;
        if (getDeviceFormat(device.cardId, device.deviceId, device.type, format)) {
            device.format = format;
            utilities::log.info(
                "Device found: {} (Card: {}, Device: {})", device.description, device.cardId, device.deviceId);
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
                (device.type == types::AudioDeviceInfo::DeviceType::kPlayback  ? "Playback"
                 : device.type == types::AudioDeviceInfo::DeviceType::kCapture ? "Capture"
                                                                               : "Invalid"));
        } else {
            utilities::log.error("Failed to get device format for card {}, device {}", device.cardId, device.deviceId);
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
        utilities::log.error("An audio device is already open. Please close it before opening a new one.");
        return false;
    }

    m_currentDevice = std::make_unique<AudioDevice>(m_alsaDriver);
    if (!m_currentDevice->open(deviceInfo)) {
        utilities::log.error("Failed to open audio device: {}", m_currentDevice->getLastError());
        m_currentDevice.reset();
        return false;
    }
    return true;
}

void AudioDeviceManager::closeDevice() {
    if (m_currentDevice && m_currentDevice->isOpen()) {
        m_currentDevice->close();
        m_currentDevice.reset();
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

    while (std::getline(cardsFile, line)) {
        std::smatch match;
        types::AudioDeviceInfo deviceInfo;
        if (std::regex_match(line, match, lineRegex)) {
            deviceInfo.cardId = std::atoi(match[1].str().c_str());
            deviceInfo.driver = match[3].str();
        } else if (std::regex_match(line, match, longnameRegex)) {
            deviceInfo.description = match[1].str();
            devices.push_back(deviceInfo);
            isDeviceFound = true;
        }
    }

    return isDeviceFound;
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
            if (cardId < devices.size()) {
                auto type{types::AudioDeviceInfo::DeviceType::kInvalid};
                if (match[3].str() == playbackId) {
                    type = types::AudioDeviceInfo::DeviceType::kPlayback;
                } else if (match[3].str() == captureId) {
                    type = types::AudioDeviceInfo::DeviceType::kCapture;
                } else {
                    utilities::log.error("Unknown device type: {}", match[3].str());
                }
                if (devices[cardId].cardId == cardId) {
                    devices[cardId].type = type;
                    devices[cardId].deviceId = std::atoi(match[2].str().c_str());
                    isDeviceFound = true;
                } else {
                    utilities::log.error("Card ID mismatch: expected {}, got {}", devices[cardId].cardId, cardId);
                    continue;  // Skip this line if card ID does not match
                }
            }
        }
    }

    return isDeviceFound;
}

const types::AudioDeviceInfo::DeviceFormat& AudioDeviceManager::getDefaultDeviceFormat() const {
    static types::AudioDeviceInfo::DeviceFormat defaultFormat{
        .periodSize = 1024,
        .periodCount = 2,
        .startTreshold = 1024,        // periodSize
        .stopTreshold = 1024 * 2,     // periodSize * periodCount
        .silenceTreshold = 1024 * 2,  // periodSize * periodCount
        .silenceSize = 0,
        .channelCount = 2,                                                  // Stereo
        .sampleRate = 44100,                                                // Common sample rate
        .sampleFormat = types::AudioDeviceInfo::DeviceFormat::kFormatS16LE  // 16-bit signed little-endian
    };
    return defaultFormat;
}

bool AudioDeviceManager::getDeviceFormat(int32_t cardId,
                                         int32_t deviceId,
                                         types::AudioDeviceInfo::DeviceType type,
                                         types::AudioDeviceInfo::DeviceFormat& format) {
    auto params = m_alsaDriver->pcmParamsGet(cardId, deviceId, type);
    if (!params) {
        return false;
    }

    const auto mask = m_alsaDriver->pcmParamsGetMask(params, AlsaDriver::kParamFormat);
    if (!mask) {
        m_alsaDriver->pcmParamsFree(params);
        return false;
    }

    auto logBuffer = std::ostringstream{};
    for (auto& bit : mask->bits) {
        logBuffer << bit << " ";
    }
    utilities::log.debug("Available formats for card[{}] device[{}]: {}", cardId, deviceId, logBuffer.str());

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
        .sampleFormat = defaultFormat.sampleFormat};
    m_alsaDriver->pcmParamsFree(params);
    return true;
}
