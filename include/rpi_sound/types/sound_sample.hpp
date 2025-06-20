#pragma once

#include <cstdint>
#include <vector>

#include "rpi_sound/types/sound_metadata.hpp"

namespace types {

using audio_t = int16_t;  // Define audio type as signed 16-bit integer

struct SoundSample {
    std::vector<audio_t> audioData;  // Pointer to the audio data
    Metadata metadata;               // Metadata about the sound sample
};

}  // namespace types
