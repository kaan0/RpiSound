#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "interfaces/iaudio_device.hpp"
#include "interfaces/iaudio_engine.hpp"
#include "types/result.hpp"
#include "types/sound_sample.hpp"

class AudioEngine : public IAudioEngine {
public:
    static constexpr size_t kMaxVoices = 8;

    explicit AudioEngine(size_t chunkSize = 512);
    ~AudioEngine() override;

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    AudioEngine(AudioEngine&&) = delete;
    AudioEngine& operator=(AudioEngine&&) = delete;

    Result<void> start(std::shared_ptr<IAudioDevice> audioDevice) override;
    Result<void> stop() override;

    // Non-blocking. Restarts if the same sample is already playing.
    // Steals the voice with the least audio remaining when all 8 slots are busy.
    // gain: 0.0 = silent, 1.0 = full volume.
    Result<void> writeSample(std::shared_ptr<const types::SoundSample> sample, float gain) override;

    bool isRunning() const override;

private:
    struct Voice {
        std::shared_ptr<const types::SoundSample> sample;
        size_t position = 0;
        float gain = 1.0f;
        bool active = false;
        // effects chain slot — populated when effect engine is added
        // std::vector<std::unique_ptr<IEffect>> effects;
    };

    std::shared_ptr<IAudioDevice> m_audioDevice;

    std::array<Voice, kMaxVoices> m_voices{};
    std::mutex m_voicesMutex;

    std::atomic<bool> m_running{false};
    std::unique_ptr<std::thread> m_mixThread;

    size_t m_chunkSize;
    std::vector<float> m_floatMix;        // float accumulator (always float, both backends)
    std::vector<types::audio_t> m_output; // converted output sent to device

    void audioThreadFunction();
};

std::unique_ptr<AudioEngine> createAudioEngine(size_t chunkSize = 512);
