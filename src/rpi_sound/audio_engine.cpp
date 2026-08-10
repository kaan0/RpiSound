#include "rpi_sound/audio_engine.hpp"

#include <algorithm>
#include <spdlog/spdlog.h>

// Converts audio_t to a normalised float in [-1, 1].
//   float  (Core Audio) : identity
//   int16_t (ALSA S16LE): divide by 32768, giving range [-1, ~1]
//   uint16_t (legacy U16LE): subtract unsigned centre then divide
static float toFloat(types::audio_t s) {
    if constexpr (std::is_floating_point_v<types::audio_t>) {
        return s;
    } else if constexpr (std::is_signed_v<types::audio_t>) {
        return static_cast<float>(s) / 32768.0f;
    } else {
        return (static_cast<float>(s) - 32768.0f) / 32768.0f;
    }
}

// Converts a normalised float in [-1, 1] back to audio_t.
static types::audio_t fromFloat(float f) {
    if constexpr (std::is_floating_point_v<types::audio_t>) {
        return f;
    } else if constexpr (std::is_signed_v<types::audio_t>) {
        return static_cast<types::audio_t>(
            std::clamp(f * 32767.0f, -32768.0f, 32767.0f));
    } else {
        return static_cast<types::audio_t>(
            std::clamp(f * 32768.0f + 32768.0f, 0.0f, 65535.0f));
    }
}

AudioEngine::AudioEngine(size_t chunkSize)
    : m_chunkSize(chunkSize),
      m_floatMix(chunkSize, 0.0f),
      m_output(chunkSize, types::audio_t{}) {
    spdlog::debug("AudioEngine created with chunk size: {}", chunkSize);
}

AudioEngine::~AudioEngine() {
    if (m_running.load(std::memory_order_acquire)) {
        stop();
    }
}

Result<void> AudioEngine::start(std::shared_ptr<IAudioDevice> audioDevice) {
    m_audioDevice = std::move(audioDevice);

    auto isOpenResult = m_audioDevice->isOpen();
    if (!isOpenResult) {
        return std::unexpected("Failed to check audio device status: " + isOpenResult.error());
    }
    if (!isOpenResult.value()) {
        auto openResult = m_audioDevice->open();
        if (!openResult) {
            return std::unexpected("Failed to open audio device: " + openResult.error());
        }
    }

    m_running.store(true, std::memory_order_release);
    m_mixThread = std::make_unique<std::thread>(&AudioEngine::audioThreadFunction, this);

    spdlog::info("Audio engine started (chunk size: {})", m_chunkSize);
    return {};
}

Result<void> AudioEngine::stop() {
    m_running.store(false, std::memory_order_release);
    if (m_mixThread && m_mixThread->joinable()) {
        m_mixThread->join();
    }
    m_mixThread.reset();
    return {};
}

Result<void> AudioEngine::writeSample(std::shared_ptr<const types::SoundSample> sample, float gain) {
    if (!sample || sample->audioData.empty()) {
        return std::unexpected("Sample is null or has no audio data");
    }

    std::lock_guard lock(m_voicesMutex);

    Voice* target = nullptr;

    // 1. Retrigger: same sample already playing → restart from the beginning.
    for (auto& v : m_voices) {
        if (v.active && v.sample && v.sample->metadata.name == sample->metadata.name) {
            target = &v;
            break;
        }
    }

    // 2. Free slot.
    if (!target) {
        for (auto& v : m_voices) {
            if (!v.active) {
                target = &v;
                break;
            }
        }
    }

    // 3. Steal the voice closest to finishing (least remaining = least audible cut).
    if (!target) {
        size_t leastRemaining = SIZE_MAX;
        for (auto& v : m_voices) {
            const size_t remaining = v.sample->audioData.size() - v.position;
            if (remaining < leastRemaining) {
                leastRemaining = remaining;
                target = &v;
            }
        }
    }

    target->sample = std::move(sample);
    target->position = 0;
    target->gain = gain;
    target->active = true;

    return {};
}

bool AudioEngine::isRunning() const {
    return m_running.load(std::memory_order_acquire);
}

void AudioEngine::audioThreadFunction() {
    spdlog::debug("Audio mixing thread started");

    while (m_running.load(std::memory_order_acquire)) {
        std::fill(m_floatMix.begin(), m_floatMix.end(), 0.0f);

        {
            std::lock_guard lock(m_voicesMutex);
            for (auto& v : m_voices) {
                if (!v.active) continue;

                const auto& data = v.sample->audioData;
                const size_t remaining = data.size() - v.position;
                const size_t toMix = std::min(remaining, m_chunkSize);

                for (size_t i = 0; i < toMix; ++i) {
                    m_floatMix[i] += toFloat(data[v.position + i]) * v.gain;
                }

                v.position += toMix;
                if (v.position >= data.size()) {
                    v.active = false;
                    v.sample.reset();  // release the shared_ptr as soon as the voice is done
                }
            }
        }

        // Clamp sum to prevent distortion on simultaneous hits.
        for (size_t i = 0; i < m_chunkSize; ++i) {
            m_output[i] = fromFloat(std::clamp(m_floatMix[i], -1.0f, 1.0f));
        }

        // Blocks for ALSA (pcm_write paces the thread); blocks on ring-buffer
        // back-pressure for Core Audio.
        auto result = m_audioDevice->write(types::audio_span_t{m_output});
        if (!result) {
            spdlog::warn("Audio write failed: {}", result.error());
        }
    }

    spdlog::debug("Audio mixing thread stopped");
}

std::unique_ptr<AudioEngine> createAudioEngine(size_t chunkSize) {
    return std::make_unique<AudioEngine>(chunkSize);
}
