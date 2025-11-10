#pragma once

#include <cstdint>
#include <string>
#include <string_view>

class ISoundManager {
public:
    // Virtual destructor
    virtual ~ISoundManager() = default;

    virtual bool initialize() = 0;

    virtual std::vector<types::AudioDeviceInfo> getAvailableAudioDevices() const = 0;

    virtual std::vector<std::string> getAvailableAudioDeviceDescriptions() const = 0;

    virtual bool selectAudioDevice(const types::AudioDeviceInfo& deviceInfo) = 0;

    // Load sound samples from the specified instrument folder
    // Returns true if the samples were successfully loaded, false otherwise
    virtual bool load(const std::string_view instrumentType) = 0;

    virtual std::vector<std::string> getAvailableSamples() const = 0;

    // Trigger a sound sample by name and velocity
    // Returns true if the sound was successfully triggered, false otherwise
    // Velocity is between 0 and 127, representing the intensity of the sound
    virtual bool triggerSound(const std::string_view sampleName, uint32_t velocity) = 0;
};
