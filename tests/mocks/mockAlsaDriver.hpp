#pragma once

#include <gmock/gmock.h>
#include "rpi_sound/alsa_driver.hpp"

class MockAlsaDriver : public AlsaDriver {
public:
    // PCM device operations
    MOCK_METHOD(PcmHandle*,
                pcmOpen,
                (uint32_t card, uint32_t device, uint32_t flags, const PcmConfig* config),
                (override));
    MOCK_METHOD(int, pcmClose, (PcmHandle * pcm), (override));
    MOCK_METHOD(int, pcmWait, (PcmHandle * pcm, int timeoutMs), (override));
    MOCK_METHOD(int, pcmWrite, (PcmHandle * pcm, const void* data, uint32_t count), (override));
    MOCK_METHOD(int, pcmRead, (PcmHandle * pcm, void* data, uint32_t count), (override));
    MOCK_METHOD(int, pcmIsReady, (const PcmHandle* pcm), (override));
    MOCK_METHOD(const char*, pcmGetError, (const PcmHandle* pcm), (override));

    // Parameter and utility methods
    MOCK_METHOD(const PcmMask*, pcmParamsGetMask, (const PcmParams* params, PcmParam param), (override));
    MOCK_METHOD(PcmParams*, pcmParamsGet, (uint32_t card, uint32_t device, Flags flags), (override));
    MOCK_METHOD(uint32_t, pcmParamsGetMax, (const PcmParams* params, PcmParam param), (override));
    MOCK_METHOD(uint32_t, pcmParamsGetMin, (const PcmParams* params, PcmParam param), (override));
    MOCK_METHOD(uint32_t, pcmFramesToBytes, (const PcmHandle* pcm, uint32_t frames), (override));
    MOCK_METHOD(uint32_t, pcmBytesToFrames, (const PcmHandle* pcm, uint32_t frames), (override));
    MOCK_METHOD(void, pcmParamsFree, (PcmParams * params), (override));
};