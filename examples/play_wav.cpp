// #include "rpi_sound/wav_parser.hpp"
// #include "rpi_sound/pcm_player.hpp"

int main() {
    #if 0
    WavParser parser("example.wav");
    if (!parser.isValid()) {
        std::cerr << "Invalid WAV file.\n";
        return 1;
    }

    PCMPlayer player;
    if (!player.play(parser.getPCMData())) {
        std::cerr << "Failed to play audio.\n";
        return 1;
    }
    #endif
    return 0;
}
