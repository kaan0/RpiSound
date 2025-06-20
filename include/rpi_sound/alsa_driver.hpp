#pragma once

extern "C" {
#include "tinyalsa/pcm.h"
}

#include <cstdint>

class AlsaDriver {
public:
    AlsaDriver() = default;
    virtual ~AlsaDriver() = default;

    // Non-copyable
    AlsaDriver(const AlsaDriver&) = delete;
    AlsaDriver& operator=(const AlsaDriver&) = delete;

    // Interface methods that wrap tinyalsa functions
    virtual struct pcm* pcmOpen(uint32_t card, uint32_t device, uint32_t flags, const struct pcm_config* config);

    virtual int pcmClose(struct pcm* pcm);

    virtual int pcmWait(struct pcm* pcm, int timeoutMs);

    virtual int pcmWrite(struct pcm* pcm, const void* data, uint32_t count);

    virtual int pcmRead(struct pcm* pcm, void* data, uint32_t count);

    virtual int pcmIsReady(const struct pcm* pcm);

    virtual const char* pcmGetError(const struct pcm* pcm);

    // Additional utility methods
    virtual int pcmGetBufferSize(const struct pcm* pcm);

    virtual int get_available_frames(const struct pcm* pcm);
};
