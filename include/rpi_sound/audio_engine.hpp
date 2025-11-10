#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <thread>
#include <vector>

#include "interfaces/iaudio_device.hpp"
#include "interfaces/iaudio_engine.hpp"
#include "types/result.hpp"
#include "types/sound_sample.hpp"

class AudioEngine : public IAudioEngine {
public:
    // Constructor
    explicit AudioEngine(size_t bufferSize = 8192, size_t chunkSize = 1024);

    // Destructor
    ~AudioEngine() override;

    // Delete copy constructor and assignment operator
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    // Move constructor and assignment operator
    AudioEngine(AudioEngine&&) = default;
    AudioEngine& operator=(AudioEngine&&) = default;

    // Start the audio engine
    Result<void> start(std::shared_ptr<IAudioDevice> audioDevice) override;

    // Stop the audio engine
    Result<void> stop() override;

    // Write samples to the circular buffer (non-blocking)
    Result<void> writeSample(const types::audio_span_t& audioData) override;

    // Check if the engine is running
    bool isRunning() const override;

private:
    std::shared_ptr<IAudioDevice> m_audioDevice;
    // Circular buffer implementation
    class CircularBuffer {
    public:
        explicit CircularBuffer(size_t size);

        // Write data to buffer (returns number of samples actually written)
        size_t write(std::span<const types::audio_t> data);

        // Read data from buffer (returns number of samples actually read)
        size_t read(std::span<types::audio_t> data);

        // Get available space for writing
        size_t getAvailableSpace() const;

        // Get available data for reading
        size_t getAvailableData() const;

        // Get total buffer size
        size_t getSize() const;

        // Check if buffer is empty
        bool isEmpty() const;

        // Clear the buffer
        void clear();
    };

    // Audio processing thread function
    void audioThreadFunction();
};

// Factory function for creating audio engines
std::unique_ptr<AudioEngine> createAudioEngine(size_t bufferSize = 8192, size_t chunkSize = 1024);