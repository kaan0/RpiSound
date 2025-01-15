#ifndef _AUDIO_DEVICE_MANAGER_HPP__
#define _AUDIO_DEVICE_MANAGER_HPP__

#include <map>
#include <memory>

#include "iaudio_device_manager.hpp"
#include "iaudio_driver.hpp"

class AudioDeviceManager : public IAudioDeviceManager {
public:
    AudioDeviceManager(std::unique_ptr<IAudioDriver> driver) :
        driver_{std::move(driver)} {}
    ~AudioDeviceManager() override = default;

    std::vector<AudioDevice> listDevices() override;
    bool setDevice(int32_t cardId, int32_t deviceId, AudioDevice::Type type) override;
    void writeData(const std::vector<uint8_t>& data) override;
 
    // move is allowed
    AudioDeviceManager(AudioDeviceManager&&) = default;
    AudioDeviceManager& operator=(AudioDeviceManager&&) = default;

    // copying is not allowed
    AudioDeviceManager(const AudioDeviceManager&) = delete;
    AudioDeviceManager& operator=(AudioDeviceManager&) = delete;

private:
    bool parseCards(std::istream& cardsFile, std::vector<AudioDevice>& devices);
    bool parseDevices(std::istream& devicesFile, std::vector<AudioDevice>& devices);
    std::unique_ptr<IAudioDriver> driver_;
    std::vector<AudioDevice> devices_;
    std::map<AudioDevice, HWAudioFormat> hardware_formats_;
};

#endif // _AUDIO_DEVICE_MANAGER_HPP__
