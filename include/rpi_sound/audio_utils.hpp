#ifndef _AUDIO_UTILS_HPP__
#define _AUDIO_UTILS_HPP__

extern "C" {
#include "tinyalsa/pcm.h"
}

#include <cstdint>
#include <tuple>

struct AudioFormat {
    uint32_t sampleRate;
    uint16_t channels;
    bool isFloat;
    uint16_t bitsPerSample;

    AudioFormat(uint32_t sr = 44100,
                uint16_t chn = 2,
                bool isf = false,
                uint16_t bps = 16) :
        sampleRate{sr},
        channels{chn},
        isFloat{isf},
        bitsPerSample{bps}{}

    AudioFormat(pcm_config&& cfg) :
        sampleRate{cfg.rate},
        channels{static_cast<uint16_t>(cfg.channels)},
        isFloat{pcmFormatToIsFloat(cfg.format)},
        bitsPerSample{pcmFormatToBits(cfg.format)} {}

    bool operator==(const AudioFormat& other) const {
        return sampleRate == other.sampleRate &&
               channels == other.channels &&
               isFloat == other.isFloat &&
               bitsPerSample == other.bitsPerSample;
    }

    bool operator!=(const AudioFormat& other) const {
        return !(*this == other);
    }

    uint16_t pcmFormatToBits(pcm_format format) const {
        return pcm_format_to_bits(format);
    }

    bool pcmFormatToIsFloat(pcm_format format) const {
        return (format == PCM_FORMAT_FLOAT_BE) ||
            (format == PCM_FORMAT_FLOAT_LE);
    }

    pcm_format toPcmFormat() {
        pcm_format fmt{PCM_FORMAT_INVALID};
        if (isFloat) {
            return PCM_FORMAT_FLOAT_LE;
        }

        switch (bitsPerSample)
        {
        case 8:
            return PCM_FORMAT_S8;
        case 16:
            return PCM_FORMAT_S16_LE;
        case 24:
            return PCM_FORMAT_S24_LE;
        case 32:
            return PCM_FORMAT_S32_LE;
        default:
            break;
        }
        return PCM_FORMAT_INVALID;
    }
};

struct PCMData {
    AudioFormat format;
    std::vector<uint8_t> data;
};

struct HWAudioFormat {
    uint32_t periodSize;
    uint32_t periodCount;
    uint32_t startTreshold;     // periodCount * periodSize
    uint32_t stopTreshold;      // periodCount * periodSize
    uint32_t silenceTreshold;   // 0
    uint32_t silenceSize;       // 0
    AudioFormat audioFormat;

    pcm_config toPcmConfig() {
        pcm_config cfg = {
            .channels = audioFormat.channels,
            .rate = audioFormat.sampleRate,
            .period_size = periodSize,
            .period_count = periodCount,
            .format = audioFormat.toPcmFormat(),
            .start_threshold = startTreshold,
            .stop_threshold = stopTreshold,
            .silence_threshold = silenceTreshold,
            .silence_size = silenceSize
        };

        return cfg;
    }
};

struct AudioDevice {
    enum Type {
        kInvalid,
        kPlayback,
        kCapture
    };
    int32_t card;
    std::vector<std::tuple<int32_t, Type, HWAudioFormat>> device;
    std::string driver;
    std::string description;
};

#endif // _AUDIO_UTILS_HPP__
