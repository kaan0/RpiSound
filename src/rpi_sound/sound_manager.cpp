#include "rpi_sound/sound_manager.hpp"

bool SoundManager::load(const std::string_view instrumentType) {
    if (!m_soundLoader->load(std::string(instrumentType))) {
        return false;  // Failed to load sound samples
    }
    return true;  // Successfully loaded sound samples
}