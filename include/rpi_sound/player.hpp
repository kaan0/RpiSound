#ifndef _PLAYER_HPP__
#define _PLAYER_HPP__

#include <memory>
#include "iaudio_device_manager.hpp"
#include "pcm_converter.hpp"

class Player {
public:
    Player() :
        converter_{std::make_unique<PCMConverter>()} {}

    bool play(const std::string_view& filePath);
    bool stop();
    void initPCM();

private:
    std::unique_ptr<IAudioDeviceManager> audio_device_;
    std::unique_ptr<PCMConverter> converter_;
    AudioFormat hw_audio_format_;
};

#endif // _PCM_PLAYER_HPP__
