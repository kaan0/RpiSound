#pragma once

#include <memory>
#include <string>
#include <vector>

#include "types/sound_sample.hpp"

class ISoundLoader {
public:
    virtual bool load(const std::string_view filePath) = 0;
    // Returns nullptr if the sample is not found.
    virtual std::shared_ptr<const types::SoundSample> getSample(std::string_view sampleName) const = 0;
    virtual std::vector<std::string> getSampleNames() const = 0;
    virtual ~ISoundLoader() = default;
};
