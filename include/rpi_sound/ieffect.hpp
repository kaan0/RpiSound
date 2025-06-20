#pragma once

#include <cstdint>
#include <span>

#include "types/sound_sample.hpp"

class IEffect {
public:
    virtual void process(std::span<types::audio_t> samples, std::size_t channels, std::size_t frames) = 0;
    virtual ~IEffect() = default;
};
