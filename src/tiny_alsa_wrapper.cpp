#include <algorithm>
#include <numeric>

#include "rpi_sound/tiny_alsa_wrapper.hpp"

bool TinyAlsaWrapper::openDevice(uint32_t card, uint32_t device, bool isOutput, HWAudioFormat& config) {
    pcm_ = std::make_unique<PCM>();

    if (!pcm_->initialize(card, device, isOutput, config)) {
        std::cout << "PCM Init failed!\r\n";
        return false;
    }
    return true;
}

bool TinyAlsaWrapper::getDeviceFormat(uint32_t card, uint32_t device, bool isOutput, HWAudioFormat& config) {
    pcm_params *params;
    const pcm_mask *mask;

    params = pcm_params_get(card, device, isOutput ? PCM_OUT : PCM_IN);

    if (!params) {
        std::cout << "\r\nCould not get parameters for card: " << card << " device: " << device << "\r\n";
        return false;
    }

    mask = pcm_params_get_mask(params, PCM_PARAM_FORMAT);
    if (mask) {
        std::cout << "Format for card[" << card << "] device[" << device << "]: ";
        for (auto& bit : mask->bits) {
            std::cout << bit << " ";
        }
        std::cout << "\r\n";
    }

    config.audioFormat.sampleRate = std::min(pcm_params_get_max(params, PCM_PARAM_RATE), 44100U);
    config.audioFormat.channels = std::min(pcm_params_get_max(params, PCM_PARAM_CHANNELS), 2U);
    config.audioFormat.bitsPerSample = pcm_params_get_max(params, PCM_PARAM_SAMPLE_BITS);
    config.periodCount = std::max(pcm_params_get_min(params, PCM_PARAM_PERIODS), 2U);
    config.periodSize = std::max(pcm_params_get_min(params, PCM_PARAM_PERIOD_SIZE), 1024U);
    config.silenceTreshold = config.periodCount * config.periodSize;
    config.startTreshold = config.periodSize;
    config.stopTreshold = config.periodCount * config.periodSize;

    pcm_params_free(params);
    return true;
}

void TinyAlsaWrapper::writeData(const std::vector<uint8_t>& data) {
    if (!pcm_) {
        std::cout << "PCM not initialized!\r\n";
        return;
    }

    auto bufferSize = static_cast<uint32_t>(pcm_frames_to_bytes(pcm_->get(), pcm_->getFormat().periodSize));
    auto remainingSize = static_cast<uint32_t>(data.size());
    uint32_t playedSize = 0;

    do {
        bufferSize = std::min(remainingSize, bufferSize);
        int32_t written_frames = pcm_writei(pcm_->get(),
                                            static_cast<const void*>(&((data)[playedSize])),
                                            pcm_bytes_to_frames(pcm_->get(),
                                            bufferSize));
        if (written_frames < 0) {
            std::cout << std::string{pcm_get_error(pcm_->get())} << "error playing sample";
            break;
        }
        remainingSize -= bufferSize;
        playedSize += pcm_frames_to_bytes(pcm_->get(), written_frames);
    } while (bufferSize > 0 && remainingSize > 0);
}

HWAudioFormat TinyAlsaWrapper::getDefaultFormat() {
    HWAudioFormat defaultFormat = {
        .periodSize = 1024,
        .periodCount = 2,
        .startTreshold = 1024 * 2,
        .stopTreshold = 1024 * 2,
        .silenceTreshold = 0,
        .silenceSize = 0,
        .audioFormat = AudioFormat{}
    };

    return defaultFormat;
}