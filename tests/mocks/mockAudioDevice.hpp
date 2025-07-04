#pragma once

#include <gmock/gmock.h>
#include "rpi_sound/iaudio_device.hpp"

class MockAudioDevice : public IAudioDevice {
public:
    // Device operations
    MOCK_METHOD(bool, open, (const types::AudioDeviceInfo& deviceInfo), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(bool, isOpen, (), (const, override));
    MOCK_METHOD(types::AudioDeviceInfo, getDeviceInfo, (), (const, override));

    // Audio operations
    MOCK_METHOD(bool, write, (const types::audio_span_t& audioData), (override));
    MOCK_METHOD(bool, read, (types::audio_span_mut_t & audioBuffer, size_t framesToRead), (override));
    MOCK_METHOD(size_t, getBufferSize, (), (const, override));
    MOCK_METHOD(size_t, getAvailableFrames, (), (const, override));

    // Error handling
    MOCK_METHOD(std::string, getLastError, (), (const, override));
};