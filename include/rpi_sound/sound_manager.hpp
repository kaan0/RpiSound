#pragma once

#include <memory>

#include "iaudio_device_manager.hpp"
#include "isound_loader.hpp"
#include "isound_manager.hpp"

class SoundManager : public ISoundManager {
public:
    static constexpr const char* kDefaultInstrumentType = "demo";

    // Constructor that initializes the sound loader and audio device manager
    SoundManager(std::unique_ptr<ISoundLoader> soundLoader, IAudioDeviceManager& audioDeviceManager)
        : m_soundLoader(std::move(soundLoader)), m_audioDeviceManager(audioDeviceManager) {}

    // Destructor
    ~SoundManager() override = default;

    bool initialize() override;

    std::vector<types::AudioDeviceInfo> getAvailableAudioDevices() const override;

    bool selectAudioDevice(const types::AudioDeviceInfo& deviceInfo) override;

    // Load sound samples from the specified instrument folder
    bool load(const std::string_view instrumentType) override;

    // Trigger a sound sample by name
    bool triggerSound(const std::string_view sampleName, uint32_t velocity) override;

private:
    // Pointer to the sound loader instance
    std::unique_ptr<ISoundLoader> m_soundLoader;

    // Reference to the audio device instance
    IAudioDeviceManager& m_audioDeviceManager;
};