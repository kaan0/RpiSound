#pragma once

#include <span>
#include <string>
#include <vector>

#include "types/audio_device_info.hpp"
#include "types/sound_sample.hpp"
#include "types/result.hpp"

using types::Result;

class IAudioDevice {
public:
    // Device operations
    virtual Result<void> open() = 0;
    virtual Result<void> close() noexcept = 0;
    virtual Result<bool> isOpen() const = 0;
    virtual types::AudioDeviceInfo& getDeviceInfo() const = 0;

    // Audio operations
    virtual Result<size_t> write(const types::audio_span_t& audioData) = 0;
    virtual Result<size_t> read(types::audio_span_mut_t& audioBuffer, size_t framesToRead) = 0;
    virtual Result<size_t> getBufferSize() const = 0;
    virtual Result<size_t> getAvailableFrames() const = 0;

    // Virtual destructor
    virtual ~IAudioDevice() = default;
};
