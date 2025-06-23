#pragma once

#include <memory>

#include "alsa_driver.hpp"
#include "iaudio_device.hpp"

class AudioDevice : public IAudioDevice {
public:
    explicit AudioDevice(std::shared_ptr<AlsaDriver> alsaDriver);
    ~AudioDevice();

    // Non-copyable
    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

    // Move constructible
    AudioDevice(AudioDevice&&) noexcept;
    AudioDevice& operator=(AudioDevice&&) noexcept;

    // Device operations
    bool open(const types::AudioDeviceInfo& deviceInfo, bool isCapture = false) override;
    void close() override;
    bool isOpen() const override;

    // Audio operations
    bool write(const types::audio_span_t& audioData) override;
    bool read(types::audio_span_mut_t& audioBuffer, size_t framesToRead) override;
    size_t getBufferSize() const;
    size_t getAvailableFrames() const;

    // Error handling
    std::string getLastError() const;

private:
    AlsaDriver::PcmConfig createPcmConfig(const types::AudioDeviceInfo::DeviceFormat& format) const;
    const types::AudioDeviceInfo::DeviceFormat& getDefaultDeviceFormat() const;
    uint32_t getFlags(bool isCapture) const;

    std::shared_ptr<AlsaDriver> m_alsaDriver;
    AlsaDriver::PcmHandle* m_pcmHandle = nullptr;
    types::AudioDeviceInfo m_deviceInfo;
    std::string m_lastError;
};
