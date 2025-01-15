#include <iostream>

#include "iaudio_device.hpp"

class Player {
private:
    IAudioDevice* hardwareDevice;

public:
    Player(IAudioDevice* device) : hardwareDevice(device) {}

    void setPlaybackDevice(const std::string& deviceName) {
        if (hardwareDevice->setDevice(deviceName)) {
            std::cout << "Device set to: " << deviceName << std::endl;
        } else {
            std::cerr << "Failed to set device." << std::endl;
        }
    }
};
