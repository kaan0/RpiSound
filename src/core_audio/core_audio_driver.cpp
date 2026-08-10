#include <algorithm>
#include <cstring>
#include <thread>
#include <chrono>

#include <AudioUnit/AudioUnit.h>

#include <spdlog/spdlog.h>

#include "core_audio/core_audio_driver.hpp"

namespace {

// Called by the CoreAudio runtime on its real-time audio thread.
// Reads pre-mixed interleaved stereo samples from the ring buffer and
// de-interleaves them into the non-interleaved output buffers (or copies
// directly into an interleaved buffer).  Outputs silence for any frames that
// the ring buffer cannot yet supply.
OSStatus renderCallback(void* inRefCon,
                        AudioUnitRenderActionFlags*,
                        const AudioTimeStamp*,
                        UInt32,
                        UInt32 nframes,
                        AudioBufferList* ioData) {
    auto* st = reinterpret_cast<CoreAudioPlayState*>(inRefCon);
    if (!st || !ioData || ioData->mNumberBuffers == 0) {
        return noErr;
    }

    constexpr int kChannels = 2;
    const size_t stereoSamples = static_cast<size_t>(nframes) * kChannels;

    if (ioData->mNumberBuffers >= 2) {
        // Non-interleaved layout: separate L and R buffers.
        auto* outL = static_cast<types::audio_t*>(ioData->mBuffers[0].mData);
        auto* outR = static_cast<types::audio_t*>(ioData->mBuffers[1].mData);

        // Read interleaved pairs into the pre-allocated scratch buffer, then
        // split into separate L/R output buffers.  Using scratch avoids both
        // dynamic allocation on the RT thread and the aliasing hazard of an
        // in-place de-interleave into outL.
        const size_t maxSamples = std::min(stereoSamples, CoreAudioPlayState::kScratchSize);
        const size_t read = st->tryRead(std::span<types::audio_t>(st->scratch.data(), maxSamples));
        const size_t framesRead = read / kChannels;

        for (size_t f = 0; f < framesRead; ++f) {
            outL[f] = st->scratch[f * kChannels];
            outR[f] = st->scratch[f * kChannels + 1];
        }
        for (size_t f = framesRead; f < nframes; ++f) {
            outL[f] = outR[f] = types::audio_t{};
        }
    } else {
        // Interleaved layout: single buffer.
        auto* dst = static_cast<types::audio_t*>(ioData->mBuffers[0].mData);
        const size_t maxSamples =
            std::min(stereoSamples,
                     ioData->mBuffers[0].mDataByteSize / sizeof(types::audio_t));
        const size_t read = st->tryRead(std::span<types::audio_t>(dst, maxSamples));
        if (read < maxSamples) {
            std::memset(dst + read, 0, (maxSamples - read) * sizeof(types::audio_t));
        }
    }

    return noErr;
}

}  // namespace

Result<HandlePtr> CoreAudioDriver::open(const types::AudioDeviceInfo& deviceInfo) {
    const auto deviceId = static_cast<CoreAudioFacade::DeviceID>(deviceInfo.deviceId);
    const auto isInput = deviceInfo.type == types::AudioDeviceInfo::DeviceType::kCapture;

    if (isInput) {
        return std::unexpected("CoreAudio driver currently supports playback only.");
    }

    CoreAudioFacade::StreamFormat fmt;
    auto fmtResult = CoreAudioUtils::getDeviceStreamFormat(deviceId, isInput, fmt);
    if (!fmtResult) {
        return std::unexpected("Failed to get device format: " + fmtResult.error());
    }

    auto unitResult = CoreAudioUtils::createDefaultOutputUnitForDevice(deviceId);
    if (!unitResult) {
        return std::unexpected("Failed to create audio unit: " + unitResult.error());
    }

    auto unit = unitResult.value();
    auto playState = std::make_shared<CoreAudioPlayState>();
    fmt.mFormatFlags = CoreAudioFacade::kFloat | CoreAudioFacade::kPacked;

    auto setFmt = CoreAudioUtils::setUnitStreamFormat(unit, isInput, fmt);
    if (!setFmt) {
        return std::unexpected("Failed to configure audio unit format: " + setFmt.error());
    }

    auto handle = HandlePtr{new CoreAudioHandle{unit, playState}};

    auto setCb = CoreAudioUtils::setRenderCallback(unit, renderCallback, playState.get());
    if (!setCb) {
        return std::unexpected("Failed to set render callback: " + setCb.error());
    }

    auto initResult = CoreAudioUtils::initializeUnit(unit);
    if (!initResult) {
        return std::unexpected("Failed to initialize audio unit: " + initResult.error());
    }

    auto startResult = CoreAudioUtils::start(unit);
    if (!startResult) {
        return std::unexpected("Failed to start audio unit: " + startResult.error());
    }

    spdlog::info("Opened CoreAudio playback device {} ({} Hz, {} channels)",
                 deviceId, fmt.mSampleRate, fmt.mChannelsPerFrame);
    return handle;
}

void CoreAudioDriver::close(const HandlePtr& handle) noexcept {
    if (audioUnit(handle)) {
        CoreAudioUtils::stop(coreAudioHandle(handle).unit);
        coreAudioHandle(handle).unit.reset();
    }
}

Result<bool> CoreAudioDriver::isOpen(const HandlePtr& handle) const {
    if (!handle || !audioUnit(handle)) {
        return std::unexpected("Invalid handle");
    }
    return true;
}

Result<size_t> CoreAudioDriver::write(const HandlePtr& handle,
                                      const types::AudioDeviceInfo& /*deviceInfo*/,
                                      const types::audio_span_t& audioData) {
    if (audioData.empty()) {
        return std::unexpected("Invalid audio data");
    }
    if (!handle || !audioUnit(handle)) {
        return std::unexpected("Invalid handle");
    }

    auto& st = *coreAudioHandle(handle).state;
    size_t written = 0;

    // Push all chunk data into the ring buffer.  When the buffer is full the
    // render callback will drain it at the hardware rate, providing natural
    // back-pressure that paces the mixing thread.
    while (written < audioData.size()) {
        const size_t n = st.tryWrite(audioData.subspan(written));
        written += n;
        if (n == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    return written * sizeof(types::audio_t);
}

Result<size_t> CoreAudioDriver::read(const HandlePtr& handle,
                                     const types::AudioDeviceInfo& /*deviceInfo*/,
                                     types::audio_span_mut_t& audioBuffer,
                                     size_t framesToRead) {
    if (!handle || !audioUnit(handle)) {
        return std::unexpected("Invalid handle");
    }
    if (audioBuffer.empty() || framesToRead == 0) {
        return std::unexpected("Invalid audio buffer or frame count");
    }
    return static_cast<size_t>(0);
}

Result<void> CoreAudioDriver::wait(const HandlePtr& handle, size_t timeoutMs) {
    if (!handle || !audioUnit(handle)) {
        return std::unexpected("Invalid handle");
    }
    if (timeoutMs == 0) {
        return std::unexpected("Invalid timeout");
    }
    return {};
}

Result<size_t> CoreAudioDriver::getBufferSize(const HandlePtr& handle) const {
    if (!handle || !audioUnit(handle)) {
        return std::unexpected("Invalid handle");
    }
    return CoreAudioPlayState::kCapacity;
}

Result<size_t> CoreAudioDriver::getAvailableFrames(const HandlePtr& handle) const {
    if (!handle || !audioUnit(handle)) {
        return std::unexpected("Invalid handle");
    }
    return coreAudioHandle(handle).state->availableToRead();
}
