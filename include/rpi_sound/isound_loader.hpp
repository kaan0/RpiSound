#pragma once

#include <string>

#include "types/sound_sample.hpp"

class ISoundLoader {
public:
    virtual bool load(const std::string& filePath) = 0;
    virtual types::SoundSample& getSample(std::string_view name) const = 0;
    virtual ~ISoundLoader() = default;
};
