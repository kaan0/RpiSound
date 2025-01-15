#ifndef _IPCM_PLAYER_HPP__
#define _IPCM_PLAYER_HPP__

#include "audio_utils.hpp"

class IPCMPlayer {
public:
    virtual ~IPCMPlayer() = default;
    virtual bool load(const std::string_view& filePath) = 0;
    virtual std::shared_ptr<std::vector<uint8_t>> getPCMData() const = 0;
    virtual AudioFormat getAudioFormat() const = 0;
};

#endif // _IPCM_PLAYER_HPP__
