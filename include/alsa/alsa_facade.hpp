#pragma once

extern "C" {
#include <tinyalsa/pcm.h>
}

#include <cstdint>

/**
 * @namespace AlsaFacade
 * @brief Wrapper for TinyALSA functionality providing audio hardware access
 * 
 * This facade provides a C++ interface to the TinyALSA library's C functions,
 * making them more accessible to the rest of the application while enabling
 * testing through dependency injection.
 */
namespace AlsaFacade {

using PcmHandle = struct pcm;
using PcmConfig = struct pcm_config;
using PcmParams = struct pcm_params;
using PcmMask = struct pcm_mask;
using PcmParam = enum pcm_param;
using PcmFormat = enum pcm_format;
using Flags = uint32_t;

static constexpr PcmParam kParamFormat = PCM_PARAM_FORMAT;
static constexpr PcmParam kParamChannels = PCM_PARAM_CHANNELS;
static constexpr PcmParam kParamRate = PCM_PARAM_RATE;
static constexpr PcmParam kParamPeriodSize = PCM_PARAM_PERIOD_SIZE;
static constexpr PcmParam kParamPeriodCount = PCM_PARAM_PERIODS;

static constexpr PcmFormat kFormatInvalid = PCM_FORMAT_INVALID;
static constexpr PcmFormat kFormatS16LE = PCM_FORMAT_S16_LE;
static constexpr PcmFormat kFormatS32LE = PCM_FORMAT_S32_LE;

static constexpr Flags kPlayback = PCM_OUT;
static constexpr Flags kCapture = PCM_IN;
static constexpr Flags kNonBlock = PCM_NONBLOCK;

/**
 * @brief Open a PCM device for playback or capture
 * 
 * @param card Card number of the PCM device
 * @param device Device number of the PCM device
 * @param flags Flags controlling device opening
 * @param config Configuration for the PCM device
 * @return PcmHandle* Handle to the opened PCM device, or nullptr on failure
 */
PcmHandle* pcmOpen(uint32_t card, uint32_t device, Flags flags, const PcmConfig* config);

int pcmClose(PcmHandle* pcm);

int pcmWait(PcmHandle* pcm, int timeoutMs);

int pcmWrite(PcmHandle* pcm, const void* data, uint32_t count);

int pcmRead(PcmHandle* pcm, void* data, uint32_t count);

bool pcmIsReady(const PcmHandle* pcm);

const char* pcmGetError(const PcmHandle* pcm);

// Additional utility methods
const PcmMask* pcmParamsGetMask(const PcmParams* params, PcmParam param);

PcmParams* pcmParamsGet(uint32_t card, uint32_t device, Flags flags);

uint32_t pcmParamsGetMax(const PcmParams* params, PcmParam param);

uint32_t pcmParamsGetMin(const PcmParams* params, PcmParam param);

int32_t pcmTestFormat(PcmParams* params, PcmFormat format);

uint32_t pcmFramesToBytes(const PcmHandle* pcm, uint32_t frames);

uint32_t pcmBytesToFrames(const PcmHandle* pcm, uint32_t frames);

void pcmParamsFree(PcmParams* params);

} //  namespace AlsaFacade
