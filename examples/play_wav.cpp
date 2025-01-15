#include <iostream>
#include <span>

#include "rpi_sound/player.hpp"

int main(int argc, char* argv[]) {

    std::span<char*> args(argv, argc);
    if (args.size() < 2) {
        std::cout << "no files provided.\r\n";
        return -1;
    }

    Player player;
    if (!player.play(args[1])) {
        std::cout << "Playing failed!\r\n";
        return -1;
    }

    return 0;
}
