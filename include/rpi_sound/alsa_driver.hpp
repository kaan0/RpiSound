#pragma once

extern "C" {
#include <tinyalsa/pcm.h>
}

#include <cstdint>

/**
 * @class AlsaDriver
 * @brief Wrapper for TinyALSA functionality providing audio hardware access
 * 
 * This class provides a C++ interface to the TinyALSA library's C functions,
 * making them more accessible to the rest of the application while enabling
 * testing through dependency injection.
 */
class AlsaDriver {
public:
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

    AlsaDriver() = default;
    virtual ~AlsaDriver() = default;

    // Non-copyable
    AlsaDriver(const AlsaDriver&) = delete;
    AlsaDriver& operator=(const AlsaDriver&) = delete;

    /**
     * @brief Open a PCM device for playback or capture
     * 
     * @param card Card number of the PCM device
     * @param device Device number of the PCM device
     * @param flags Flags controlling device opening
     * @param config Configuration for the PCM device
     * @return PcmHandle* Handle to the opened PCM device, or nullptr on failure
     */
    virtual PcmHandle* pcmOpen(uint32_t card, uint32_t device, uint32_t flags, const PcmConfig* config);

    virtual int pcmClose(PcmHandle* pcm);

    virtual int pcmWait(PcmHandle* pcm, int timeoutMs);

    virtual int pcmWrite(PcmHandle* pcm, const void* data, uint32_t count);

    virtual int pcmRead(PcmHandle* pcm, void* data, uint32_t count);

    virtual bool pcmIsReady(const PcmHandle* pcm);

    virtual const char* pcmGetError(const PcmHandle* pcm);

    // Additional utility methods
    virtual const PcmMask* pcmParamsGetMask(const PcmParams* params, PcmParam param);

    virtual PcmParams* pcmParamsGet(uint32_t card, uint32_t device, Flags flags);

    virtual uint32_t pcmParamsGetMax(const PcmParams* params, PcmParam param);

    virtual uint32_t pcmParamsGetMin(const PcmParams* params, PcmParam param);

    virtual uint32_t pcmFramesToBytes(const PcmHandle* pcm, uint32_t frames);

    virtual uint32_t pcmBytesToFrames(const PcmHandle* pcm, uint32_t frames);

    virtual void pcmParamsFree(PcmParams* params);
};
