#pragma once

#include <cstdint>
#include <string>

namespace types {

struct Metadata {
    std::string name;     // Name of the sound file
    uint32_t sampleRate;  // Sample rate of the sound file (e.g., 44100 Hz)
    std::size_t frames;   // Number of audio frames
    uint16_t channels;    // Number of audio channels (e.g., 1 for mono, 2 for stereo)
};

}  // namespace types
