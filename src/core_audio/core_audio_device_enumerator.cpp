#include <AudioUnit/AudioUnit.h>
#include <spdlog/spdlog.h>

#include "core_audio/core_audio_device_enumerator.hpp"
#include "core_audio/core_audio_utils.hpp"

Result<std::vector<types::AudioDeviceInfo>> CoreAudioDeviceEnumerator::list(types::AudioDeviceInfo::DeviceType type) {
    std::vector<types::AudioDeviceInfo> devices;
    auto devicesSize = CoreAudioUtils::getPropertySize(kAudioObjectSystemObject, &CoreAudioFacade::kAllDevices);
    if (!devicesSize) {
        return std::unexpected(devicesSize.error());
    }
    auto count = devicesSize.value() / sizeof(AudioDeviceID);
    std::vector<AudioDeviceID> devices_vector(count);
    auto status = CoreAudioUtils::getProperty(
        kAudioObjectSystemObject, &CoreAudioFacade::kAllDevices, devicesSize.value(), devices_vector.data());

    if (!status) {
        return std::unexpected(status.error());
    }

    for (auto& device : devices_vector) {
        // Check if it has output channels
        AudioBufferList audio_device;
        auto inAddress = &CoreAudioFacade::kOutStreamConfig;
        if (type == types::AudioDeviceInfo::DeviceType::kCapture) {
            inAddress = &CoreAudioFacade::kInputStreamConfig;
        }
        auto result = CoreAudioUtils::getProperty(device, inAddress, sizeof(AudioBufferList), &audio_device);
        if (!result || !audio_device.mNumberBuffers) {
            continue;
        }
        auto deviceNameSize = CoreAudioUtils::getPropertySize(device, &CoreAudioFacade::kDeviceName);
        if (!deviceNameSize) {
            continue;
        }
        std::string name(deviceNameSize.value(), '\0');
        auto deviceName =
            CoreAudioUtils::getProperty(device, &CoreAudioFacade::kDeviceName, deviceNameSize.value(), name.data());

        if (!deviceName) {
            continue;
        }

        types::AudioDeviceInfo deviceInfo{
            .cardId = 0, .deviceId = static_cast<int32_t>(device), .type = type, .description = name};

        auto formatResult = getDeviceFormat(deviceInfo);
        if (!formatResult) {
            spdlog::error("Format error: {}", formatResult.error());
            continue;
        }

        deviceInfo.format = formatResult.value();
        devices.push_back(deviceInfo);

        spdlog::info("{} device name: {}", types::AudioDeviceInfo::to_string(type), name);
        spdlog::info("Format: {} SampleRate: {}",
                     types::AudioDeviceInfo::to_string(deviceInfo.format.sampleFormat),
                     deviceInfo.format.sampleRate);
        spdlog::info("{} device buffers: {}", types::AudioDeviceInfo::to_string(type), audio_device.mNumberBuffers);
    }

    return devices;
}

// CoreAudio: "probing" is different. You generally query stream format + nominal sample rate range,
// and channels via the current StreamFormat. Here's a practical equivalent.

Result<types::AudioDeviceInfo::DeviceFormat> CoreAudioDeviceEnumerator::getDeviceFormat(
    const types::AudioDeviceInfo& deviceInfo) const {

    // 1) Current stream format (output vs input scope)
    CoreAudioFacade::StreamFormat asbd{};
    {
        const bool isInput = (deviceInfo.type == types::AudioDeviceInfo::DeviceType::kCapture);
        auto r = CoreAudioUtils::getDeviceStreamFormat(
            static_cast<CoreAudioFacade::DeviceID>(deviceInfo.deviceId), isInput, asbd);
        if (!r) {
            return std::unexpected("Failed to get device stream format: " + r.error());
        }
    }

    auto ranges = CoreAudioUtils::getPropertyArray<AudioValueRange>(
        static_cast<CoreAudioFacade::ObjectID>(deviceInfo.deviceId), &CoreAudioFacade::kNominalSampleRateRange);
    if (!ranges) {
        spdlog::error("Failed to get sample rates!");
        return std::unexpected("Failed to get sample rates!" + ranges.error());
    }

    // 3) Sample format probing: CoreAudio exposes it via ASBD flags/bits.
    // Map to your enum (adjust mapping to your actual enum values)
    types::AudioDeviceInfo::DeviceFormat::SampleFormat sampleFormat =
        types::AudioDeviceInfo::DeviceFormat::kFormatInvalid;
    const bool isFloat = (asbd.mFormatFlags & kAudioFormatFlagIsFloat) != 0;
    const bool isInt = (asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger) != 0;

    if (asbd.mFormatID == kAudioFormatLinearPCM) {
        if (isInt && asbd.mBitsPerChannel == 16) {
            sampleFormat = types::AudioDeviceInfo::DeviceFormat::kFormatS16LE;
        } else if (isInt && asbd.mBitsPerChannel == 32) {
            sampleFormat = types::AudioDeviceInfo::DeviceFormat::kFormatS32LE;
        } else if (isFloat && asbd.mBitsPerChannel == 32) {
            sampleFormat = types::AudioDeviceInfo::DeviceFormat::kFormatFloat;
        }
    }

    const auto def = CoreAudioUtils::getDefaultDeviceFormat();  // your default policy

    types::AudioDeviceInfo::DeviceFormat fmt{
        .periodSize = def.periodSize,    // CoreAudio prefers buffer frames set on the AudioUnit, not device
        .periodCount = def.periodCount,  // keep policy/default
        .startTreshold = def.startTreshold,
        .stopTreshold = def.stopTreshold,
        .silenceTreshold = def.silenceTreshold,
        .silenceSize = def.silenceSize,
        .channelCount =
            std::min<uint32_t>(asbd.mChannelsPerFrame ? asbd.mChannelsPerFrame : def.channelCount, def.channelCount),
        .sampleRate = static_cast<uint32_t>(std::min<double>(ranges.value().back().mMaximum, def.sampleRate)),
        .sampleFormat = sampleFormat};

    return fmt;
}
