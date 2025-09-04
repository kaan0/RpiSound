#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

#include "types/audio_device_info.hpp"
#include "types/sound_sample.hpp"
#include "types/result.hpp"

struct IAudioHandle {
    virtual ~IAudioHandle() = default;
};
using HandlePtr = std::shared_ptr<IAudioHandle>;
using types::Result;

class IAudioDriver {
public:
    // Device operations
    virtual Result<HandlePtr> open(const types::AudioDeviceInfo& deviceInfo) = 0;
    virtual void close(const HandlePtr&) noexcept = 0;
    virtual Result<bool> isOpen(const HandlePtr&) const = 0;

    // Audio operations
    virtual Result<size_t> write(const HandlePtr& handle, const types::audio_span_t& audioData) = 0;
    virtual Result<size_t> read(const HandlePtr& handle, types::audio_span_mut_t& audioBuffer, size_t framesToRead) = 0;
    virtual Result<void> wait(const HandlePtr& handle, size_t timeoutMs) = 0;
    virtual Result<size_t> getBufferSize(const HandlePtr& handle) const = 0;
    virtual Result<size_t> getAvailableFrames(const HandlePtr& handle) const = 0;

    // Virtual destructor
    virtual ~IAudioDriver() = default;
};
 