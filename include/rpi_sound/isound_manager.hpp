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

    virtual bool selectAudioDevice(const types::AudioDeviceInfo& deviceInfo) = 0;

    // Load sound samples from the specified instrument folder
    // Returns true if the samples were successfully loaded, false otherwise
    virtual bool load(const std::string_view instrumentType) = 0;

    // Trigger a sound sample by name and velocity
    // Returns true if the sound was successfully triggered, false otherwise
    // Velocity is typically a value between 0 and 127, representing the intensity of the sound
    // For example, a velocity of 0 means no sound, while 127 means
    virtual bool triggerSound(const std::string_view sampleName, uint32_t velocity) = 0;
};
