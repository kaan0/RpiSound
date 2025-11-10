#include "rpi_sound/audio_engine.hpp"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>

AudioEngine::CircularBuffer::CircularBuffer(size_t size) {}

size_t AudioEngine::CircularBuffer::write(std::span<const types::audio_t> data) {}

size_t AudioEngine::CircularBuffer::read(std::span<types::audio_t> data) {}

size_t AudioEngine::CircularBuffer::getAvailableSpace() const {}

size_t AudioEngine::CircularBuffer::getAvailableData() const {}

void AudioEngine::CircularBuffer::clear() {}

AudioEngine::AudioEngine(size_t bufferSize, size_t chunkSize) {
    spdlog::debug("AudioEngine created with buffer size: {}, chunk size: {}", bufferSize, chunkSize);
}

AudioEngine::~AudioEngine() {}

Result<void> AudioEngine::start(std::shared_ptr<IAudioDevice> audioDevice) {
    m_audioDevice = std::move(audioDevice);

    // Check if audio device is open
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

    // Start audio thread
    // std::make_unique<std::thread>(&AudioEngine::audioThreadFunction, this);

    spdlog::info("Audio engine started successfully");
    return {};
}

Result<void> AudioEngine::stop() {
    return {};
}

Result<void> AudioEngine::writeSample(const types::audio_span_t& audioData) {
    m_audioDevice->write(audioData);
    return {};
}

bool AudioEngine::isRunning() const {
    return false;
}

void AudioEngine::audioThreadFunction() {
    spdlog::debug("Audio thread started");

    spdlog::debug("Audio thread stopped");
}

std::unique_ptr<AudioEngine> createAudioEngine(size_t bufferSize, size_t chunkSize) {
    return std::make_unique<AudioEngine>(bufferSize, chunkSize);
}