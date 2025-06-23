#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace types {

using audio_t = int16_t;                        // Audio type as signed 16-bit integer
using audio_span_t = std::span<const audio_t>;  // Non-owning, non-modifying view of audio data
using audio_span_mut_t = std::span<audio_t>;    // Non-owning, modifiable view of audio data

struct SoundSample {
    struct Metadata {
        std::string name;     // Name of the sound file
        uint32_t sampleRate;  // Sample rate of the sound file (e.g., 44100 Hz)
        std::size_t frames;   // Number of audio frames
        uint16_t channels;    // Number of audio channels (e.g., 1 for mono, 2 for stereo)
    };
    std::vector<audio_t> audioData;  // Pointer to the audio data
    Metadata metadata;               // Metadata about the sound sample

    // Span view of the audio data
    [[nodiscard]] audio_span_t getAudioSpan() const noexcept { return audio_span_t(audioData); }

    // Mutable span view of the audio data
    [[nodiscard]] audio_span_mut_t getAudioSpanMut() noexcept { return audio_span_mut_t(audioData); }
};

}  // namespace types
