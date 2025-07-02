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
    m_pcmHandle = m_alsaDriver->pcmOpen(deviceInfo.cardId, deviceInfo.deviceId, deviceInfo.type, &config);
    if (!m_pcmHandle) {
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

    // Get buffer size in frames
    auto bufferSizeFrames = m_deviceInfo.format.periodSize;
    auto channelCount = m_deviceInfo.format.channelCount;
    auto samplesPerFrame = channelCount;

    // Calculate how many samples we can write per chunk
    uint32_t maxSamplesPerChunk = bufferSizeFrames * samplesPerFrame;

    size_t totalSamples = audioData.size();
    size_t samplesWritten = 0;

    utilities::log.debug(
        "Writing {} samples ({} frames) to audio device with buffer size {} frames, {} samples per chunk",
        totalSamples,
        totalSamples / samplesPerFrame,
        bufferSizeFrames,
        maxSamplesPerChunk);

    while (samplesWritten < totalSamples) {
        // Calculate how many samples to write in this chunk
        uint32_t remainingSamples = totalSamples - samplesWritten;
        auto samplesToWrite = std::min(maxSamplesPerChunk, remainingSamples);

        // Ensure we write complete frames (samples must be divisible by channel count)
        auto framesToWrite = samplesToWrite / samplesPerFrame;
        if (framesToWrite == 0 && remainingSamples > 0) {
            // If we have less than one frame remaining, pad or handle appropriately
            utilities::log.warning("Not enough samples to write a full frame. Remaining samples: {}", remainingSamples);
            break;
        }

        samplesToWrite = framesToWrite * samplesPerFrame;

        // Get the chunk of data to write
        auto chunk = audioData.subspan(samplesWritten, samplesToWrite);

        // pcmWrite expects data as bytes, but takes frame count as parameter
        auto result = m_alsaDriver->pcmWrite(m_pcmHandle, reinterpret_cast<const char*>(chunk.data()), framesToWrite);

        if (result < 0) {
            m_lastError = m_alsaDriver->pcmGetError(m_pcmHandle);
            utilities::log.error("Failed to write {} frames to PCM device: {}", framesToWrite, m_lastError);
            return false;
        }

        // result is the number of frames actually written
        auto actualSamplesWritten = static_cast<size_t>(result) * samplesPerFrame;
        samplesWritten += actualSamplesWritten;

        // If we couldn't write the full chunk, we might need to wait or handle underrun
        if (static_cast<size_t>(result) < framesToWrite) {
            utilities::log.warning("Partial write: requested {} frames, wrote {} frames", framesToWrite, result);

            // Wait for the device to be ready for more data
            auto waitResult = m_alsaDriver->pcmWait(m_pcmHandle, 1000);  // 1 second timeout
            if (waitResult < 0) {
                m_lastError = m_alsaDriver->pcmGetError(m_pcmHandle);
                utilities::log.error("Failed to wait for PCM device: {}", m_lastError);
                return false;
            }
        }
    }

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
