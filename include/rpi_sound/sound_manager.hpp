#pragma once

#include <memory>

#include "iaudio_device.hpp"
#include "isound_loader.hpp"
#include "isound_manager.hpp"

class SoundManager : public ISoundManager {
public:
    // Constructor that initializes the sound loader and audio device
    SoundManager(std::unique_ptr<ISoundLoader> soundLoader, std::unique_ptr<IAudioDevice> audioDevice)
        : m_soundLoader(std::move(soundLoader)), m_audioDevice(std::move(audioDevice)) {}

    // Load sound samples from the specified instrument folder
    bool load(const std::string_view instrumentType) override;

    // Trigger a sound sample by name
    bool triggerSound(const std::string_view sampleName, uint32_t velocity) override;

    // Destructor
    ~SoundManager() override = default;

private:
    // Pointer to the sound loader instance
    std::unique_ptr<ISoundLoader> m_soundLoader;

    // Pointer to the audio device instance
    std::unique_ptr<IAudioDevice> m_audioDevice;
};