#include <gmock/gmock.h>

class MockAudioDevice : public IAudioDevice {
public:
    MOCK_METHOD(std::vector<std::string>, listDevices, (), (const, override));
    MOCK_METHOD(bool, setDevice, (const std::string& deviceName), (override));
    MOCK_METHOD(DeviceParams, getDeviceParams, (const std::string& deviceName), (const, override));
};
