#pragma once

#include "alsa_facade.hpp"
#include "types/audio_device_info.hpp"

namespace alsa_utils
{

inline AlsaFacade::Flags toAlsaFlag(types::AudioDeviceInfo::DeviceType type) {
        return (type == types::AudioDeviceInfo::DeviceType::kPlayback) ? AlsaFacade::kPlayback : AlsaFacade::kCapture;
}

inline AlsaFacade::PcmConfig createPcmConfig(const types::AudioDeviceInfo::DeviceFormat& format) {
    AlsaFacade::PcmConfig config{.channels = format.channelCount,
                                 .rate = format.sampleRate,
                                 .period_size = format.periodSize,
                                 .period_count = format.periodCount,
                                 .format = static_cast<AlsaFacade::PcmFormat>(format.sampleFormat),
                                 .start_threshold = format.startTreshold,
                                 .stop_threshold = format.stopTreshold,
                                 .silence_threshold = format.silenceTreshold,
                                 .silence_size = format.silenceSize,
                                 .avail_min = 0};
    return config;
}

inline const types::AudioDeviceInfo::DeviceFormat& getDefaultDeviceFormat() {
    static types::AudioDeviceInfo::DeviceFormat defaultFormat{
        .periodSize = 1024,
        .periodCount = 2,
        .startTreshold = 1024,     // periodSize
        .stopTreshold = 1024 * 2,  // periodSize * periodCount
        .silenceTreshold = 0,
        .silenceSize = 0,
        .channelCount = 2,                                                  // Stereo
        .sampleRate = 44100,                                                // Common sample rate
        .sampleFormat = types::AudioDeviceInfo::DeviceFormat::kFormatS32LE  // 16-bit signed little-endian
    };
    return defaultFormat;
}

} // namespace alsa_utils
