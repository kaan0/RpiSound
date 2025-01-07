#include <iostream>

#include "rpi_sound/wav_parser.hpp"
// #include "rpi_sound/pcm_player.h"

int main() {
    
    WavParser parser;
    if (!parser.load("pcm3244s.wav")) {
        std::cerr << "Invalid WAV file.\n";
        return 1;
    }
    #if 0
    PCMPlayer player;
    if (!player.play(parser.getPCMData())) {
        std::cerr << "Failed to play audio.\n";
        return 1;
    }
    #endif
    return 0;
}
