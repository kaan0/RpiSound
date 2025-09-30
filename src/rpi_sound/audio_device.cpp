#include <iostream>
#include <numeric>

#include <spdlog/spdlog.h>

#include "rpi_sound/audio_device.hpp"

AudioDevice::AudioDevice(IAudioDriver& audioDriver, const types::AudioDeviceInfo& deviceInfo) : m_audioDriver{audioDriver}, m_deviceInfo{deviceInfo} {}

AudioDevice::~AudioDevice() {
    auto is_open = isOpen();
    if (is_open && is_open.value()) {
        close();
    }
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
        spdlog::warn("Audio device is already open.");
        return std::unexpected("Audio device is already open.");
    }

    auto result = m_audioDriver.open(m_deviceInfo);
    if (!result) {
        spdlog::error("Failed to open audio device: {}", result.error());
        return std::unexpected(result.error());
    }

    m_driverHandle = result.value();
    return {};
}

Result<void> AudioDevice::close() noexcept {
    if (!isOpen()) {
        spdlog::warn("Audio device is not open. Nothing to close.");
        return std::unexpected("Audio device is not open. Nothing to close.");
    }
    m_audioDriver.close(m_driverHandle);
    m_driverHandle = nullptr;
    m_deviceInfo = {};

    return {};
}

Result<bool> AudioDevice::isOpen() const {
    if (!m_driverHandle) {
        return std::unexpected("Device not initialized yet.");
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

    return m_audioDriver.write(m_driverHandle, m_deviceInfo, audioData);
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
