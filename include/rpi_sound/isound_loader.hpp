#pragma once

#include <string>

#include "types/sound_sample.hpp"

class ISoundLoader {
public:
    virtual bool load(const std::string& filePath) = 0;
    virtual const types::SoundSample& getSample(const std::string_view sampleName) const = 0;
    virtual ~ISoundLoader() = default;
};
