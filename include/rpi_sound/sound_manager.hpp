#pragma once

#include <memory>

#include "interfaces/iaudio_device_manager.hpp"
#include "interfaces/iaudio_engine.hpp"
#include "interfaces/isound_loader.hpp"
#include "interfaces/isound_manager.hpp"

class SoundManager : public ISoundManager {
public:
    static constexpr const char* kDefaultInstrumentType = "demo";

    // Constructor that initializes the sound loader and audio device manager
    SoundManager(std::unique_ptr<ISoundLoader> soundLoader,
                 IAudioDeviceManager& audioDeviceManager,
                 std::unique_ptr<IAudioEngine> audioEngine)
        : m_soundLoader(std::move(soundLoader)),
          m_audioDeviceManager(audioDeviceManager),
          m_audioEngine(std::move(audioEngine)) {}

    // Destructor
    ~SoundManager() override = default;

    bool initialize() override;

    std::vector<types::AudioDeviceInfo> getAvailableAudioDevices(
        types::AudioDeviceInfo::DeviceType type) const override;

    std::vector<std::string> getAvailableAudioDeviceDescriptions(
        types::AudioDeviceInfo::DeviceType type) const override;

    std::vector<std::string> getAvailableSamples() const override { return m_soundLoader->getSampleNames(); }

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

    // Pointer to the audio engine
    std::unique_ptr<IAudioEngine> m_audioEngine;
};