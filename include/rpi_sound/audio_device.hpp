#pragma once

#include <memory>

#include "interfaces/iaudio_device.hpp"
#include "interfaces/iaudio_driver.hpp"

class AudioDevice : public IAudioDevice {
public:
    explicit AudioDevice(IAudioDriver& audioDriver, const types::AudioDeviceInfo& deviceInfo);
    ~AudioDevice() override;

    // Non-copyable
    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

    // Move constructible
    AudioDevice(AudioDevice&&) noexcept;
    AudioDevice& operator=(AudioDevice&&) noexcept;

    // Device operations
    Result<void> open() override;
    Result<void> close() noexcept override;
    Result<bool> isOpen() const override;
    Result<types::AudioDeviceInfo> getDeviceInfo() const override;

    // Audio operations
    Result<size_t> write(const types::audio_span_t& audioData) override;
    Result<size_t> read(types::audio_span_mut_t& audioBuffer, size_t framesToRead) override;
    Result<size_t> getBufferSize() const override;
    Result<size_t> getAvailableFrames() const override;

private:
    IAudioDriver& m_audioDriver;
    HandlePtr m_driverHandle;
    types::AudioDeviceInfo m_deviceInfo;
};
