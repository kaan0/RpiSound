#pragma once

#include "interfaces/iaudio_device.hpp"

#include "types/result.hpp"

class IAudioEngine {
public:
    virtual Result<void> writeSample(const types::audio_span_t& audioData) = 0;
    // Start the audio engine
    virtual Result<void> start(std::shared_ptr<IAudioDevice> audioDevice) = 0;

    // Stop the audio engine
    virtual Result<void> stop() = 0;

    virtual bool isRunning() const = 0;

    virtual ~IAudioEngine() = default;
};
