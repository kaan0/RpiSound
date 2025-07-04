#include "rpi_sound/alsa_driver.hpp"

AlsaDriver::PcmHandle* AlsaDriver::pcmOpen(uint32_t card, uint32_t device, uint32_t flags, const PcmConfig* config) {
    return pcm_open(card, device, flags, config);
}

int AlsaDriver::pcmClose(PcmHandle* pcm) {
    return pcm_close(pcm);
}

int AlsaDriver::pcmWait(PcmHandle* pcm, int timeoutMs) {
    return pcm_wait(pcm, timeoutMs);
}

int AlsaDriver::pcmWrite(PcmHandle* pcm, const void* data, uint32_t count) {
    return pcm_writei(pcm, data, count);
}

int AlsaDriver::pcmRead(PcmHandle* pcm, void* data, uint32_t count) {
    return pcm_read(pcm, data, count);
}

int AlsaDriver::pcmIsReady(const PcmHandle* pcm) {
    return pcm_is_ready(pcm);
}

const char* AlsaDriver::pcmGetError(const PcmHandle* pcm) {
    return pcm_get_error(pcm);
}

const AlsaDriver::PcmMask* AlsaDriver::pcmParamsGetMask(const PcmParams* params, PcmParam param) {
    return pcm_params_get_mask(params, param);
}

AlsaDriver::PcmParams* AlsaDriver::pcmParamsGet(uint32_t card, uint32_t device, Flags flags) {
    return pcm_params_get(card, device, flags);
}

uint32_t AlsaDriver::pcmParamsGetMax(const PcmParams* params, PcmParam param) {
    return pcm_params_get_max(params, param);
}

uint32_t AlsaDriver::pcmParamsGetMin(const PcmParams* params, PcmParam param) {
    return pcm_params_get_min(params, param);
}

uint32_t AlsaDriver::pcmFramesToBytes(const PcmHandle* pcm, uint32_t frames) {
    return pcm_frames_to_bytes(pcm, frames);
}

uint32_t AlsaDriver::pcmBytesToFrames(const PcmHandle* pcm, uint32_t bytes) {
    return pcm_bytes_to_frames(pcm, bytes);
}

void AlsaDriver::pcmParamsFree(PcmParams* params) {
    pcm_params_free(params);
}
