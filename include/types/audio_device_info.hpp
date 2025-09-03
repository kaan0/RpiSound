#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace types {

struct AudioDeviceInfo {
    enum DeviceType { kInvalid, kPlayback, kCapture };
    // Convert DeviceType to string
    static std::string to_string(DeviceType type) {
        switch (type) {
            case kInvalid:
                return "Invalid";
            case kPlayback:
                return "Playback";
            case kCapture:
                return "Capture";
            default:
                return "Unknown";
        }
    }
    struct DeviceFormat {
        enum SampleFormat {
            kFormatInvalid = -1,  // Invalid format
            kFormatS16LE = 0,     // 16-bit signed little-endian
            kFormatS32LE = 1,     // 32-bit signed little-endian
            kFormatFloat = 9,     // 32-bit floating point
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
    int32_t cardId;
    int32_t deviceId;
    DeviceType type;
    DeviceFormat format;
    std::string driver;
    std::string description;
};

}  // namespace types
