#include <algorithm>

#include <spdlog/spdlog.h>

#include "alsa/alsa_driver.hpp"

Result<HandlePtr> AlsaDriver::open(const types::AudioDeviceInfo& deviceInfo) {
    auto config{alsa_utils::createPcmConfig(deviceInfo.format)};
    auto* handle{AlsaFacade::pcmOpen(deviceInfo.cardId, deviceInfo.deviceId, alsa_utils::toAlsaFlag(deviceInfo.type), &config)};
    if (!AlsaFacade::pcmIsReady(handle)) {
        return std::unexpected(std::string(AlsaFacade::pcmGetError(handle)));
    }
    return HandlePtr{ new AlsaHandle{handle} };
}

void AlsaDriver::close(const HandlePtr& handle) noexcept {
    if (auto* p = ph(handle)) {
        AlsaFacade::pcmClose(p);
        ah(handle).p = nullptr;
    }
}

Result<bool> AlsaDriver::isOpen(const HandlePtr& handle) const {
    if (!handle || !ph(handle)) {
        return std::unexpected("Invalid handle");
    }
    return AlsaFacade::pcmIsReady(ph(handle));
}

Result<size_t> AlsaDriver::write(const HandlePtr& handle, const types::AudioDeviceInfo& deviceInfo, const types::audio_span_t& audioData) {
    if (audioData.empty()) {
        return std::unexpected("Invalid audio data");
    } else if (!handle || !ph(handle)) {
        return std::unexpected("Invalid handle");
    }

    size_t remainingBytes{sizeof(types::audio_t) * audioData.size()};
    size_t remainingFrames{AlsaFacade::pcmBytesToFrames(ph(handle), remainingBytes)};
    auto writtenFrames{AlsaFacade::pcmWrite(ph(handle), reinterpret_cast<const void*>(audioData.data()), remainingFrames)};
    if (writtenFrames < remainingFrames) {
        return std::unexpected("Underrun occured.");
    } else if (writtenFrames < 0) {
        return std::unexpected("Writing to audio device failed.");
    }

    return remainingBytes;
}

Result<size_t> AlsaDriver::read(const HandlePtr& handle, const types::AudioDeviceInfo& deviceInfo, types::audio_span_mut_t& audioBuffer, size_t framesToRead) {
    if (!handle || !ph(handle)) {
        return std::unexpected("Invalid handle");
    }
    if (audioBuffer.empty() || framesToRead == 0) {
        return std::unexpected("Invalid audio buffer or frame count");
    }
    return static_cast<size_t>(AlsaFacade::pcmRead(ph(handle), reinterpret_cast<void*>(audioBuffer.data()), static_cast<uint32_t>(framesToRead)));
}

Result<void> AlsaDriver::wait(const HandlePtr& handle, size_t timeoutMs) {
    if (!handle || !ph(handle)) {
        return std::unexpected("Invalid handle");
    }
    if (timeoutMs == 0) {
        return std::unexpected("Invalid timeout");
    }
    AlsaFacade::pcmWait(ph(handle), timeoutMs);
    return {};
}

Result<size_t> AlsaDriver::getBufferSize(const HandlePtr& handle) const {
    if (!handle || !ph(handle)) {
        return std::unexpected("Invalid handle");
    }
    return 0;
}

Result<size_t> AlsaDriver::getAvailableFrames(const HandlePtr& handle) const {
    if (!handle || !ph(handle)) {
        return std::unexpected("Invalid handle");
    }
    return 0;
}
