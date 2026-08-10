#include "core_audio/core_audio_utils.hpp"

types::Result<uint32_t> CoreAudioUtils::getPropertySize(CoreAudioFacade::ObjectID inObjectID,
                                                        const CoreAudioFacade::ObjectAddr* inAddress) {
    uint32_t size{0};
    auto status = CoreAudioFacade::getPropertyDataSize(inObjectID, inAddress, 0, nullptr, &size);
    if (status != noErr) {
        return std::unexpected("Getting data size failed with code: " + std::to_string(static_cast<int>(status)));
    }
    return size;
}

types::Result<uint32_t> CoreAudioUtils::getProperty(CoreAudioFacade::ObjectID inObjectID,
                                                    const CoreAudioFacade::ObjectAddr* inAddress,
                                                    uint32_t dataSize,
                                                    void* data) {
    if (!inAddress || dataSize == 0 || !data) {
        return std::unexpected("Parameter error!");
    }

    auto size = CoreAudioUtils::getPropertySize(inObjectID, inAddress);
    if (!size) {
        return std::unexpected(size.error());
    }

    if (size.value() > dataSize) {
        return std::unexpected("Insufficient input data size!");
    }

    uint32_t readSize{dataSize};
    auto status = CoreAudioFacade::getPropertyData(inObjectID, inAddress, 0, nullptr, &readSize, data);
    if (status != noErr) {
        return std::unexpected(std::string("Getting data failed with code: ") +
                               std::string(CoreAudioUtils::osStatusToString(status)));
    }
    return readSize;
}

types::Result<void> CoreAudioUtils::setProperty(CoreAudioFacade::ObjectID inObjectID,
                                                const CoreAudioFacade::ObjectAddr* inAddress,
                                                uint32_t dataSize,
                                                void* data) {
    if (!inAddress || dataSize == 0 || !data) {
        return std::unexpected("Parameter error!");
    }

    auto status = CoreAudioFacade::setPropertyData(inObjectID, inAddress, 0, nullptr, dataSize, data);

    if (status != noErr) {
        return std::unexpected(std::string("Setting data failed with code: ") +
                               std::string(CoreAudioUtils::osStatusToString(status)));
    }
    return {};
}

types::Result<void> CoreAudioUtils::setProperty(CoreAudioFacade::UnitHandle unit,
                                                CoreAudioFacade::UnitPropertyID propertyId,
                                                CoreAudioFacade::UnitScope scope,
                                                uint32_t dataSize,
                                                void* data) {
    if (!unit || dataSize == 0 || !data) {
        return std::unexpected("Parameter error!");
    }

    auto status = CoreAudioFacade::setUnitProperty(unit, propertyId, scope, 0, data, dataSize);

    if (status != noErr) {
        return std::unexpected(std::string("Setting data failed with code: ") +
                               std::string(CoreAudioUtils::osStatusToString(status)));
    }

    return {};
}

types::Result<void> CoreAudioUtils::getDefaultOutputDevice(CoreAudioFacade::DeviceID& outDeviceId) {
    auto size{sizeof(CoreAudioFacade::DeviceID)};
    auto status = getProperty(kAudioObjectSystemObject, &CoreAudioFacade::kDefaultOutputDevice, size, &outDeviceId);
    if (!status) {
        return std::unexpected("Getting default out device failed: " + status.error());
    }

    return {};
}

types::Result<void> CoreAudioUtils::getDefaultInputDevice(CoreAudioFacade::DeviceID& inputDeviceId) {
    auto size{sizeof(CoreAudioFacade::DeviceID)};
    auto status = getProperty(kAudioObjectSystemObject, &CoreAudioFacade::kDefaultInputDevice, size, &inputDeviceId);
    if (!status) {
        return std::unexpected("Getting default input device failed: " + status.error());
    }

    return {};
}

types::Result<std::string> CoreAudioUtils::getDeviceName(CoreAudioFacade::DeviceID deviceId) {
    auto deviceNameSize = getPropertySize(deviceId, &CoreAudioFacade::kDeviceName);
    if (!deviceNameSize) {
        return std::unexpected("Getting device name size failed: " + deviceNameSize.error());
    }
    std::string name(deviceNameSize.value(), '\0');
    auto deviceName = getProperty(deviceId, &CoreAudioFacade::kDeviceName, deviceNameSize.value(), name.data());

    if (!deviceName) {
        return std::unexpected("Getting device name failed: " + deviceName.error());
    }

    return name;
}

types::Result<void> CoreAudioUtils::getDeviceStreamFormat(CoreAudioFacade::DeviceID deviceId,
                                                          bool isInput,
                                                          CoreAudioFacade::StreamFormat& outFormat) {
    auto size{sizeof(CoreAudioFacade::StreamFormat)};
    auto addr = &CoreAudioFacade::kOutStreamFormat;
    if (isInput) {
        addr = &CoreAudioFacade::kInputStreamFormat;
    }
    auto status = getProperty(deviceId, addr, size, &outFormat);

    if (!status) {
        return std::unexpected("Getting device format failed: " + status.error());
    }

    return {};
}

types::Result<void> CoreAudioUtils::setUnitStreamFormat(const CoreAudioFacade::UnitPtr& unit,
                                                        bool isInput,
                                                        CoreAudioFacade::StreamFormat& format) {
    auto size{sizeof(CoreAudioFacade::StreamFormat)};
    auto scope = CoreAudioFacade::kUnitScopeOutput;
    if (isInput) {
        scope = CoreAudioFacade::kUnitScopeInput;
    }
    CoreAudioFacade::UnitHandle raw = CoreAudioFacade::toRaw(unit);
    if (!raw) {
        return std::unexpected("setUnitStreamFormat: unit is null.");
    }
    auto status = setProperty(raw, CoreAudioFacade::kUnitStreamFormat, scope, size, &format);
    if (!status) {
        return std::unexpected("Setting device format failed: " + status.error());
    }

    return {};
}

types::Result<CoreAudioFacade::UnitPtr> CoreAudioUtils::createOutputUnit(
    const CoreAudioFacade::ComponentFormat& format) {

    CoreAudioFacade::Component* component = CoreAudioFacade::findNext(nullptr, format);

    if (!component) {
        return std::unexpected("Couldn't find next component.");
    }

    CoreAudioFacade::UnitHandle rawUnit = nullptr;

    auto status = CoreAudioFacade::newInstance(*component, rawUnit);
    if (status != noErr) {
        return std::unexpected(std::string("Creating new unit failed with code: ") +
                               std::string(CoreAudioUtils::osStatusToString(status)));
    }

    return CoreAudioFacade::wrapUnit(rawUnit);
}

types::Result<CoreAudioFacade::UnitPtr> CoreAudioUtils::createDefaultOutputUnit() {
    // TODO: Check kAudioUnitSubType_HALOutput -> kAudioUnitSubType_DefaultOutput
    CoreAudioFacade::ComponentFormat fmt{.componentType = kAudioUnitType_Output,
                                         .componentSubType = kAudioUnitSubType_HALOutput,
                                         .componentManufacturer = kAudioUnitManufacturer_Apple};
    return createOutputUnit(fmt);
}

types::Result<void> CoreAudioUtils::bindUnitToDevice(const CoreAudioFacade::UnitPtr& unit,
                                                     CoreAudioFacade::DeviceID deviceId) {
    CoreAudioFacade::UnitHandle raw = CoreAudioFacade::toRaw(unit);
    if (!raw) {
        return std::unexpected("bindUnitToDevice: unit is null.");
    }

    // Enable output on bus 0
    uint32_t enableIO = 1;

    auto status =
        setProperty(raw, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, sizeof(enableIO), &enableIO);
    if (!status) {
        return std::unexpected(std::string("Enable output IO failed: ") + status.error());
    }

    // Bind to specific device
    status =
        setProperty(raw, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, sizeof(deviceId), &deviceId);
    if (!status) {
        return std::unexpected(std::string("Bind unit to device failed: ") + status.error());
    }

    return {};
}

types::Result<CoreAudioFacade::UnitPtr> CoreAudioUtils::createDefaultOutputUnitForDevice(
    CoreAudioFacade::DeviceID deviceId) {
    auto unitRes = createDefaultOutputUnit();
    if (!unitRes) {
        return std::unexpected(unitRes.error());
    }

    auto bindRes = bindUnitToDevice(*unitRes, deviceId);
    if (!bindRes) {
        return std::unexpected(bindRes.error());
    }

    return *unitRes;
}

types::Result<void> CoreAudioUtils::initializeUnit(const CoreAudioFacade::UnitPtr& unit) {
    auto raw = CoreAudioFacade::toRaw(unit);
    if (!raw) {
        return std::unexpected("initializeUnit parameter error!");
    }
    auto status = CoreAudioFacade::init(raw);
    if (status != noErr) {
        std::unexpected("initializeUnit error: " + std::string(CoreAudioUtils::osStatusToString(status)));
    }
    return {};
}

types::Result<void> CoreAudioUtils::uninitializeUnit(const CoreAudioFacade::UnitPtr& unit) {
    auto raw = CoreAudioFacade::toRaw(unit);
    if (!raw) {
        return std::unexpected("uninitializeUnit parameter error!");
    }
    auto status = CoreAudioFacade::deInit(raw);
    if (status != noErr) {
        std::unexpected("uninitializeUnit error: " + std::string(CoreAudioUtils::osStatusToString(status)));
    }
    return {};
}

types::Result<void> CoreAudioUtils::start(const CoreAudioFacade::UnitPtr& unit) {
    auto raw = CoreAudioFacade::toRaw(unit);
    if (!raw) {
        return std::unexpected("start parameter error!");
    }
    auto status = CoreAudioFacade::startOutput(raw);
    if (status != noErr) {
        std::unexpected("start error: " + std::string(CoreAudioUtils::osStatusToString(status)));
    }
    return {};
}

types::Result<void> CoreAudioUtils::stop(const CoreAudioFacade::UnitPtr& unit) {
    auto raw = CoreAudioFacade::toRaw(unit);
    if (!raw) {
        return std::unexpected("stop parameter error!");
    }
    auto status = CoreAudioFacade::stopOutput(raw);
    if (status != noErr) {
        std::unexpected("stop error: " + std::string(CoreAudioUtils::osStatusToString(status)));
    }
    return {};
}

types::Result<void> CoreAudioUtils::dispose(const CoreAudioFacade::UnitPtr& unit) {
    auto raw = CoreAudioFacade::toRaw(unit);
    if (!raw) {
        return std::unexpected("dispose parameter error!");
    }
    auto status = CoreAudioFacade::deleteInstance(raw);
    if (status != noErr) {
        std::unexpected("dispose error: " + std::string(CoreAudioUtils::osStatusToString(status)));
    }
    return {};
}

types::Result<void> CoreAudioUtils::setRenderCallback(const CoreAudioFacade::UnitPtr& unit,
                                                      AURenderCallback callback,
                                                      void* userData) {
    auto raw = CoreAudioFacade::toRaw(unit);
    if (!raw || !callback) {
        return std::unexpected("dispose parameter error!");
    }

    AURenderCallbackStruct cb{};
    cb.inputProc = callback;
    cb.inputProcRefCon = userData;

    return setProperty(raw, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, sizeof(cb), &cb);
}

uint32_t CoreAudioUtils::framesToBytes(const CoreAudioFacade::StreamFormat& format, uint32_t frames) {
    auto bpf = static_cast<uint32_t>(format.mBytesPerFrame);
    if (bpf == 0) {
        if (format.mBitsPerChannel == 0 || format.mChannelsPerFrame == 0) {
            return 0;
        }
        bpf = (format.mBitsPerChannel / 8u) * format.mChannelsPerFrame;
    }
    return frames * bpf;
}

uint32_t CoreAudioUtils::bytesToFrames(const CoreAudioFacade::StreamFormat& format, uint32_t bytes) {
    auto bpf = static_cast<uint32_t>(format.mBytesPerFrame);
    if (bpf == 0) {
        if (format.mBitsPerChannel == 0 || format.mChannelsPerFrame == 0) {
            return 0;
        }
        bpf = (format.mBitsPerChannel / 8u) * format.mChannelsPerFrame;
    }
    if (bpf == 0) {
        return 0;
    }
    return bytes / bpf;
}

const types::AudioDeviceInfo::DeviceFormat& CoreAudioUtils::getDefaultDeviceFormat() {
    static types::AudioDeviceInfo::DeviceFormat defaultFormat{
        .periodSize = 1024,
        .periodCount = 2,
        .startTreshold = 1024,     // periodSize
        .stopTreshold = 1024 * 2,  // periodSize * periodCount
        .silenceTreshold = 0,
        .silenceSize = 0,
        .channelCount = 2,                                                  // Stereo
        .sampleRate = 44100,                                                // Common sample rate
        .sampleFormat = types::AudioDeviceInfo::DeviceFormat::kFormatFloat  // 32-bit float little-endian
    };
    return defaultFormat;
}

const char* CoreAudioUtils::osStatusToString(CoreAudioFacade::CoreStatus status) {
    switch (status) {
        case noErr:
            return "noErr";
        case kAudio_ParamError:
            return "kAudio_ParamError";
        case kAudioUnitErr_InvalidProperty:
            return "kAudioUnitErr_InvalidProperty";
        case kAudioUnitErr_InvalidParameter:
            return "kAudioUnitErr_InvalidParameter";
        case kAudioUnitErr_InvalidElement:
            return "kAudioUnitErr_InvalidElement";
        case kAudioUnitErr_NoConnection:
            return "kAudioUnitErr_NoConnection";
        case kAudioUnitErr_FailedInitialization:
            return "kAudioUnitErr_FailedInitialization";
        case kAudioUnitErr_TooManyFramesToProcess:
            return "kAudioUnitErr_TooManyFramesToProcess";
        case kAudioUnitErr_InvalidFile:
            return "kAudioUnitErr_InvalidFile";
        case kAudioUnitErr_UnknownFileType:
            return "kAudioUnitErr_UnknownFileType";
        case kAudioUnitErr_FileNotSpecified:
            return "kAudioUnitErr_FileNotSpecified";
        case kAudioUnitErr_FormatNotSupported:
            return "kAudioUnitErr_FormatNotSupported";
        case kAudioUnitErr_Uninitialized:
            return "kAudioUnitErr_Uninitialized";
        case kAudioUnitErr_InvalidScope:
            return "kAudioUnitErr_InvalidScope";
        case kAudioUnitErr_PropertyNotWritable:
            return "kAudioUnitErr_PropertyNotWritable";
        default:
            return "kAudioUnitErr_Unknown";
    }
}