#pragma once

#include <cstdint>
#include <string>
#include <string_view>

class ISoundManager {
public:
    // Load sound samples from the specified instrument folder
    virtual bool load(const std::string_view instrumentType) = 0;

    // Trigger a sound sample by name
    virtual bool triggerSound(const std::string_view sampleName, uint32_t velocity) = 0;

    virtual ~ISoundManager() = default;
};
