#pragma once

#include "types/audio_device_info.hpp"
#include "types/result.hpp"
#include "types/sound_sample.hpp"

#include "interfaces/iaudio_driver.hpp"

#include "alsa/alsa_facade.hpp"
#include "alsa/alsa_utils.hpp"

struct AlsaHandle final : IAudioHandle {
    explicit AlsaHandle(AlsaFacade::PcmHandle* pcm) : p{pcm} {}
    ~AlsaHandle() override {
        if (p) {
            AlsaFacade::pcmClose(p);
        }
    }
    AlsaFacade::PcmHandle* p;
};

class AlsaDriver final : public IAudioDriver {
public:
    // Device operations
    Result<HandlePtr> open(const types::AudioDeviceInfo& deviceInfo) override;
    void close(const HandlePtr& handle) noexcept override;
    Result<bool> isOpen(const HandlePtr& handle) const override;

    // Audio operations
    Result<size_t> write(const HandlePtr& handle,
                         const types::AudioDeviceInfo& deviceInfo,
                         const types::audio_span_t& audioData) override;
    Result<size_t> read(const HandlePtr& handle,
                        const types::AudioDeviceInfo& deviceInfo,
                        types::audio_span_mut_t& audioBuffer,
                        size_t framesToRead) override;
    Result<void> wait(const HandlePtr& handle, size_t timeoutMs) override;
    Result<size_t> getBufferSize(const HandlePtr& handle) const override;
    Result<size_t> getAvailableFrames(const HandlePtr& handle) const override;

private:
    static AlsaHandle& ah(const HandlePtr& h) { return static_cast<AlsaHandle&>(*h); }
    static AlsaFacade::PcmHandle* ph(const HandlePtr& h) { return ah(h).p; }
};
