#pragma once

#include <memory>

#include "interfaces/iaudio_device.hpp"
#include "types/result.hpp"
#include "types/sound_sample.hpp"

class IAudioEngine {
public:
    // gain: 0.0 = silent, 1.0 = full volume (maps directly from MIDI velocity / 127).
    virtual Result<void> writeSample(std::shared_ptr<const types::SoundSample> sample, float gain) = 0;
    virtual Result<void> start(std::shared_ptr<IAudioDevice> audioDevice) = 0;
    virtual Result<void> stop() = 0;
    virtual bool isRunning() const = 0;
    virtual ~IAudioEngine() = default;
};
