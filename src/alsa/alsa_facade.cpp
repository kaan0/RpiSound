#include "alsa/alsa_facade.hpp"

AlsaFacade::PcmHandle* AlsaFacade::pcmOpen(uint32_t card, uint32_t device, Flags flags, const PcmConfig* config) {
    return pcm_open(card, device, flags, config);
}

int AlsaFacade::pcmClose(PcmHandle* pcm) {
    return pcm_close(pcm);
}

int AlsaFacade::pcmWait(PcmHandle* pcm, int timeoutMs) {
    return pcm_wait(pcm, timeoutMs);
}

int AlsaFacade::pcmWrite(PcmHandle* pcm, const void* data, uint32_t count) {
    return pcm_writei(pcm, data, count);
}

int AlsaFacade::pcmRead(PcmHandle* pcm, void* data, uint32_t count) {
    return pcm_read(pcm, data, count);
}

bool AlsaFacade::pcmIsReady(const PcmHandle* pcm) {
    return pcm_is_ready(pcm) != 0;
}

const char* AlsaFacade::pcmGetError(const PcmHandle* pcm) {
    return pcm_get_error(pcm);
}

const AlsaFacade::PcmMask* AlsaFacade::pcmParamsGetMask(const PcmParams* params, PcmParam param) {
    return pcm_params_get_mask(params, param);
}

AlsaFacade::PcmParams* AlsaFacade::pcmParamsGet(uint32_t card, uint32_t device, Flags flags) {
    return pcm_params_get(card, device, flags);
}

uint32_t AlsaFacade::pcmParamsGetMax(const PcmParams* params, PcmParam param) {
    return pcm_params_get_max(params, param);
}

uint32_t AlsaFacade::pcmParamsGetMin(const PcmParams* params, PcmParam param) {
    return pcm_params_get_min(params, param);
}

int32_t AlsaFacade::pcmTestFormat(PcmParams* params, PcmFormat format) {
    return pcm_params_format_test(params, format);
}

uint32_t AlsaFacade::pcmFramesToBytes(const PcmHandle* pcm, uint32_t frames) {
    return pcm_frames_to_bytes(pcm, frames);
}

uint32_t AlsaFacade::pcmBytesToFrames(const PcmHandle* pcm, uint32_t bytes) {
    return pcm_bytes_to_frames(pcm, bytes);
}

void AlsaFacade::pcmParamsFree(PcmParams* params) {
    pcm_params_free(params);
}
