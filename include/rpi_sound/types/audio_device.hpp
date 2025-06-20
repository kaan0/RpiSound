#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace types {

struct AudioDeviceFormat {
    enum SampleFormat {
        kFormatS16LE,   // 16-bit signed little-endian
        kFormatS32LE,   // 32-bit signed little-endian
        kFormatFloat,   // 32-bit floating point
        kFormatInvalid  // Invalid format
    };
    uint32_t periodSize;
    uint32_t periodCount;
    uint32_t startTreshold;     // periodCount * periodSize
    uint32_t stopTreshold;      // periodCount * periodSize
    uint32_t silenceTreshold;   // 0
    uint32_t silenceSize;       // 0
    uint32_t channelCount;      // 1 for mono, 2 for stereo
    uint32_t sampleRate;        // e.g., 44100 Hz
    SampleFormat sampleFormat;  // e.g., 16-bit signed little-endian
};

struct AudioDevice {
    enum Type { kInvalid, kPlayback, kCapture };
    int32_t cardId;
    int32_t deviceId;
    Type type;
    std::string driver;
    std::string description;
};

}  // namespace types
