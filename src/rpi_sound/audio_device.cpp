#include <iostream>
#include <numeric>

#include "rpi_sound/audio_device.hpp"
#include "utilities/logger.hpp"

AudioDevice::AudioDevice(std::shared_ptr<AlsaDriver> alsaDriver) : m_alsaDriver(std::move(alsaDriver)) {}

AudioDevice::~AudioDevice() {
    close();
}

AudioDevice::AudioDevice(AudioDevice&& other) noexcept
    : m_alsaDriver(std::move(other.m_alsaDriver)),
      m_pcmHandle(other.m_pcmHandle),
      m_deviceInfo(std::move(other.m_deviceInfo)),
      m_lastError(std::move(other.m_lastError)) {

    other.m_pcmHandle = nullptr;
}

AudioDevice& AudioDevice::operator=(AudioDevice&& other) noexcept {
    if (this != &other) {
        close();

        m_alsaDriver = std::move(other.m_alsaDriver);
        m_pcmHandle = other.m_pcmHandle;
        m_deviceInfo = std::move(other.m_deviceInfo);
        m_lastError = std::move(other.m_lastError);

        other.m_pcmHandle = nullptr;
    }
    return *this;
}

bool AudioDevice::open(const types::AudioDeviceInfo& deviceInfo) {
    auto config{createPcmConfig(deviceInfo.format)};
    m_pcmHandle = m_alsaDriver->pcmOpen(deviceInfo.cardId, deviceInfo.deviceId, toAlsaFlag(deviceInfo.type), &config);
    if (!m_alsaDriver->pcmIsReady(m_pcmHandle)) {
        m_lastError = m_alsaDriver->pcmGetError(m_pcmHandle);
        return false;
    }
    m_deviceInfo = deviceInfo;
    return true;
}

void AudioDevice::close() {
    static_cast<void>(m_alsaDriver->pcmClose(m_pcmHandle));
    m_pcmHandle = nullptr;
    m_deviceInfo = {};
}

bool AudioDevice::isOpen() const {
    return m_pcmHandle != nullptr;
}

bool AudioDevice::write(const types::audio_span_t& audioData) {
    if (!m_pcmHandle || audioData.empty()) {
        return false;
    }

    // Convert audio data to bytes (assuming audio_span_t contains int16_t or similar)
    const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(audioData.data());
    auto totalDataSize = static_cast<uint32_t>(audioData.size() * sizeof(typename types::audio_span_t::element_type));

    auto bufferSize =
        static_cast<uint32_t>(m_alsaDriver->pcmFramesToBytes(m_pcmHandle, m_deviceInfo.format.periodSize));
    auto remainingSize = totalDataSize;
    uint32_t playedSize = 0;

    utilities::log.debug("Writing {} bytes to audio device with buffer size {} bytes", totalDataSize, bufferSize);

    do {
        auto currentBufferSize = std::min(remainingSize, bufferSize);

        int32_t writtenFrames = m_alsaDriver->pcmWrite(m_pcmHandle,
                                                       reinterpret_cast<const char*>(&dataPtr[playedSize]),
                                                       m_alsaDriver->pcmBytesToFrames(m_pcmHandle, currentBufferSize));

        if (writtenFrames < 0) {
            m_lastError = m_alsaDriver->pcmGetError(m_pcmHandle);
            utilities::log.error("Error playing sample: {}", m_lastError);
            return false;
        }

        auto writtenBytes = static_cast<uint32_t>(m_alsaDriver->pcmFramesToBytes(m_pcmHandle, writtenFrames));
        remainingSize -= writtenBytes;
        playedSize += writtenBytes;

    } while (remainingSize > 0);

    return true;
}

bool AudioDevice::read(types::audio_span_mut_t& audioBuffer, size_t framesToRead) {
    // Empty implementation
    return false;
}

size_t AudioDevice::getBufferSize() const {
    return static_cast<size_t>(m_alsaDriver->pcmFramesToBytes(m_pcmHandle, m_deviceInfo.format.periodSize));
}

size_t AudioDevice::getAvailableFrames() const {
    // Empty implementation
    return 0;
}

std::string AudioDevice::getLastError() const {
    return m_lastError;
}

AlsaDriver::PcmConfig AudioDevice::createPcmConfig(const types::AudioDeviceInfo::DeviceFormat& format) const {
    AlsaDriver::PcmConfig config{.channels = format.channelCount,
                                 .rate = format.sampleRate,
                                 .period_size = format.periodSize,
                                 .period_count = format.periodCount,
                                 .format = static_cast<AlsaDriver::PcmFormat>(format.sampleFormat),
                                 .start_threshold = format.startTreshold,
                                 .stop_threshold = format.stopTreshold,
                                 .silence_threshold = format.silenceTreshold,
                                 .silence_size = format.silenceSize,
                                 .avail_min = 0};
    return config;
}
