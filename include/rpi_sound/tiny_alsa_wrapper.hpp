#ifndef _TINY_ALSA_WRAPPER_HPP__
#define _TINY_ALSA_WRAPPER_HPP__

#include <memory>
#include <iostream>

#include "iaudio_driver.hpp"
#include "audio_utils.hpp"

class PCM {
public:
    PCM() : pcm_(nullptr) {}
    ~PCM() {
        if (pcm_) {
            pcm_wait(pcm_, 1000);
            pcm_close(pcm_);
        }
    }
    // copying is not allowed
    PCM(const PCM&) = delete;
    PCM& operator=(const PCM&) = delete;

    // move is allowed
    PCM(PCM&& other) noexcept : pcm_(other.pcm_) {
        other.pcm_ = nullptr;
    }
    PCM& operator=(PCM&& other) noexcept {
        if (this != &other) {
            if (pcm_) {
                pcm_wait(pcm_, 1000);
                pcm_close(pcm_);
            }
            pcm_ = other.pcm_;
            other.pcm_ = nullptr;
        }
        return *this;
    }

    bool initialize(uint32_t card, uint32_t device, bool isOutput, HWAudioFormat& config) {
        if (!pcm_) {
            pcm_config pcmConfig = config.toPcmConfig();
            pcm_ = pcm_open(card,
                device,
                isOutput ? PCM_OUT : PCM_IN,
                &pcmConfig);
            if (!pcm_is_ready(pcm_)) {
                std::cout << "PCM open failed!\r\n";
                pcm_close(pcm_);
                pcm_ = nullptr;
                return false;
            }
        }
        format = config;
        return true;
    }
    pcm* get() const {
        return pcm_;
    }

    HWAudioFormat getFormat() {
        return format;
    }
private:
    HWAudioFormat format;
    pcm* pcm_;
};

class TinyAlsaWrapper : public IAudioDriver {
public:
    ~TinyAlsaWrapper() override = default;
    bool openDevice(uint32_t card, uint32_t device, bool isOutput, HWAudioFormat& config) override;
    bool getDeviceFormat(uint32_t card, uint32_t device, bool isOutput, HWAudioFormat& config) override;
    void writeData(const std::vector<uint8_t>& data) override;
    HWAudioFormat getDefaultFormat() override;

private:
    std::unique_ptr<PCM> pcm_;
};

#endif // _TINY_ALSA_WRAPPER_HPP__
