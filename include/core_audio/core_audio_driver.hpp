#pragma once

#include <array>
#include <atomic>
#include <algorithm>

#include "types/audio_device_info.hpp"
#include "types/result.hpp"
#include "types/sound_sample.hpp"

#include "core_audio/core_audio_utils.hpp"
#include "interfaces/iaudio_driver.hpp"

// Ring buffer shared between the AudioEngine mixing thread (writer) and the
// CoreAudio render callback (reader).  Both ends run on different threads so
// all positions are atomic.  Capacity is a power-of-two for cheap modulo.
struct CoreAudioPlayState {
    static constexpr size_t kCapacity = 8192;  // samples (not frames)
    // Pre-allocated scratch for the render callback to de-interleave into L/R
    // without dynamic allocation on the real-time audio thread.
    // 2048 samples covers 1024 stereo frames — larger than any typical hardware buffer.
    static constexpr size_t kScratchSize = 2048;

    std::array<types::audio_t, kCapacity + 1> buffer{};
    std::array<types::audio_t, kScratchSize> scratch{};
    std::atomic<size_t> readPos{0};
    std::atomic<size_t> writePos{0};

    size_t availableToRead() const noexcept {
        const size_t r = readPos.load(std::memory_order_acquire);
        const size_t w = writePos.load(std::memory_order_acquire);
        return (w + kCapacity + 1 - r) % (kCapacity + 1);
    }

    size_t availableToWrite() const noexcept {
        return kCapacity - availableToRead();
    }

    // Returns how many samples were actually written (may be less than data.size()).
    size_t tryWrite(std::span<const types::audio_t> data) noexcept {
        const size_t toWrite = std::min(data.size(), availableToWrite());
        const size_t w = writePos.load(std::memory_order_relaxed);
        for (size_t i = 0; i < toWrite; ++i) {
            buffer[(w + i) % (kCapacity + 1)] = data[i];
        }
        writePos.store((w + toWrite) % (kCapacity + 1), std::memory_order_release);
        return toWrite;
    }

    // Returns how many samples were actually read (may be less than out.size()).
    size_t tryRead(std::span<types::audio_t> out) noexcept {
        const size_t toRead = std::min(out.size(), availableToRead());
        const size_t r = readPos.load(std::memory_order_relaxed);
        for (size_t i = 0; i < toRead; ++i) {
            out[i] = buffer[(r + i) % (kCapacity + 1)];
        }
        readPos.store((r + toRead) % (kCapacity + 1), std::memory_order_release);
        return toRead;
    }
};

struct CoreAudioHandle final : IAudioHandle {
    explicit CoreAudioHandle(CoreAudioFacade::UnitPtr u, std::shared_ptr<CoreAudioPlayState> s)
        : unit{std::move(u)}, state{std::move(s)} {}
    CoreAudioFacade::UnitPtr unit;
    std::shared_ptr<CoreAudioPlayState> state;
};

class CoreAudioDriver final : public IAudioDriver {
public:
    Result<HandlePtr> open(const types::AudioDeviceInfo& deviceInfo) override;
    void close(const HandlePtr& handle) noexcept override;
    Result<bool> isOpen(const HandlePtr& handle) const override;

    // Streams audioData into the ring buffer.  Blocks only when the buffer is
    // full, providing natural back-pressure to the mixing thread.
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
    static CoreAudioHandle& coreAudioHandle(const HandlePtr& h) { return static_cast<CoreAudioHandle&>(*h); }

    static CoreAudioFacade::UnitHandle audioUnit(const HandlePtr& h) {
        return CoreAudioFacade::toRaw(coreAudioHandle(h).unit);
    }
};
