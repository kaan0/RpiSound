#include <iostream>
#include <numeric>

#include "rpi_sound/audio_device.hpp"
#include "utilities/logger.hpp"

AudioDevice::AudioDevice(IAudioDriver& audioDriver, const types::AudioDeviceInfo& deviceInfo) : m_audioDriver(audioDriver), m_deviceInfo(deviceInfo) {}

AudioDevice::~AudioDevice() {
    close();
}

AudioDevice::AudioDevice(AudioDevice&& other) noexcept
    : m_audioDriver{other.m_audioDriver},
      m_driverHandle{other.m_driverHandle},
      m_deviceInfo{other.m_deviceInfo} {

    other.m_driverHandle = nullptr;
}

AudioDevice& AudioDevice::operator=(AudioDevice&& other) noexcept {
    if (this != &other) {
        close();

        m_audioDriver = other.m_audioDriver;
        m_driverHandle = other.m_driverHandle;
        m_deviceInfo = other.m_deviceInfo;

        other.m_driverHandle = nullptr;
        other.m_deviceInfo = {};

    }
    return *this;
}

Result<void> AudioDevice::open() {
    if (isOpen()) {
        utilities::log.warning("Audio device is already open.");
        return std::unexpected("Audio device is already open.");
    }

    auto result = m_audioDriver.open(m_deviceInfo);
    if (!result) {
        utilities::log.error("Failed to open audio device: {}", result.error());
        return std::unexpected(result.error());
    }

    m_driverHandle = result.value();
}

Result<void> AudioDevice::close() noexcept {
    if (!isOpen()) {
        utilities::log.warning("Audio device is not open. Nothing to close.");
        return std::unexpected("Audio device is not open. Nothing to close.");
    }
    m_audioDriver.close(m_driverHandle);
    m_driverHandle = nullptr;
    m_deviceInfo = {};
}

Result<bool> AudioDevice::isOpen() const {
    if (!m_driverHandle) {
        return false;
    }
    return m_audioDriver.isOpen(m_driverHandle);
}

Result<types::AudioDeviceInfo> AudioDevice::getDeviceInfo() const {
    if (!m_driverHandle) {
        return std::unexpected("Device not open.");
    }

    return m_deviceInfo;
}

Result<size_t> AudioDevice::write(const types::audio_span_t& audioData) {
    if (!m_driverHandle || audioData.empty()) {
        return std::unexpected("Invalid driver handle or empty audio data");
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
        auto result = m_audioDriver.write(m_driverHandle, chunk);

        if (!result) {
            utilities::log.error("Failed to write {} frames to PCM device: {}", framesToWrite, result.error());
            return std::unexpected(result.error());
        }

        // result is the number of frames actually written
        auto actualSamplesWritten = static_cast<size_t>(result.value()) * samplesPerFrame;
        samplesWritten += actualSamplesWritten;

        // If we couldn't write the full chunk, we might need to wait or handle underrun
        if (static_cast<size_t>(result.value()) < framesToWrite) {
            utilities::log.warning("Partial write: requested {} frames, wrote {} frames", framesToWrite, result.value());

            // Wait for the device to be ready for more data
            auto waitResult = m_audioDriver.wait(m_driverHandle, 1000);  // 1 second timeout
            if (!waitResult) {
                utilities::log.error("Failed to wait for PCM device: {}", waitResult.error());
                return std::unexpected(waitResult.error());
            }
        }
    }

    return true;
}

Result<size_t> AudioDevice::read(types::audio_span_mut_t& audioBuffer, size_t framesToRead) {
    return false;
}

Result<size_t> AudioDevice::getBufferSize() const {
    if (!m_driverHandle) {
        return std::unexpected("Driver handle is null");
    }
    return m_audioDriver.getBufferSize(m_driverHandle);
}

Result<size_t> AudioDevice::getAvailableFrames() const {
    if (!m_driverHandle) {
        return std::unexpected("Driver handle is null");
    }
    return m_audioDriver.getAvailableFrames(m_driverHandle);
}
