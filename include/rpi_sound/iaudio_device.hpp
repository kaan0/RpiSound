#pragma once

#include <span>
#include <string>
#include <vector>

#include "types/audio_device_info.hpp"
#include "types/sound_sample.hpp"

class IAudioDevice {
public:
    // Device operations
    virtual bool open(const types::AudioDeviceInfo& deviceInfo, bool isCapture = false) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    // Audio operations
    virtual bool write(const types::audio_span_t& audioData) = 0;
    virtual bool read(types::audio_span_mut_t& audioBuffer, size_t framesToRead) = 0;
    virtual size_t getBufferSize() const = 0;
    virtual size_t getAvailableFrames() const = 0;

    // Error handling
    virtual std::string getLastError() const = 0;

    // Virtual destructor
    virtual ~IAudioDevice() = default;
};
