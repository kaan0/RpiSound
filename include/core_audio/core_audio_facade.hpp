#pragma once

#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>

#include <memory>
#include <type_traits>

/**
 * @namespace CoreAudioFacade
 * @brief Wrapper for CoreAudio functionality providing audio hardware access
 *
 * This facade provides a C++ interface to the CoreAudio C APIs, making them
 * more accessible to the rest of the application while enabling testing
 * through dependency injection.
 */
namespace CoreAudioFacade {

using CoreStatus = ::OSStatus;
using DeviceID = ::AudioDeviceID;
using ObjectID = ::AudioObjectID;
using UnitPropertyID = ::AudioUnitPropertyID;
using UnitScope = ::AudioUnitScope;
using UnitElement = ::AudioUnitElement;
using ObjectAddr = ::AudioObjectPropertyAddress;
using UnitHandle = ::AudioUnit;
using UnitPtr = std::shared_ptr<void>;
using StreamFormat = ::AudioStreamBasicDescription;
using ComponentFormat = ::AudioComponentDescription;
using Component = std::remove_pointer_t<::AudioComponent>;
using ComponentInstance = std::remove_pointer_t<::AudioComponentInstance>;
using Flags = uint32_t;

struct StreamConfig {
    double sampleRate{};
    uint32_t channels{};
    uint32_t framesPerBuffer{};
};

static constexpr ObjectAddr kAllDevices{kAudioHardwarePropertyDevices,
                                        kAudioObjectPropertyScopeGlobal,
                                        kAudioObjectPropertyElementMain};

static constexpr ObjectAddr kDefaultOutputDevice{kAudioHardwarePropertyDefaultOutputDevice,
                                                 kAudioObjectPropertyScopeGlobal,
                                                 kAudioObjectPropertyElementMain};

static constexpr ObjectAddr kDefaultInputDevice{kAudioHardwarePropertyDefaultInputDevice,
                                                kAudioObjectPropertyScopeGlobal,
                                                kAudioObjectPropertyElementMain};

static constexpr ObjectAddr kDeviceName{kAudioDevicePropertyDeviceName,
                                        kAudioObjectPropertyScopeGlobal,
                                        kAudioObjectPropertyElementMain};

static constexpr ObjectAddr kOutStreamConfig{kAudioDevicePropertyStreamConfiguration,
                                             kAudioDevicePropertyScopeOutput,
                                             kAudioObjectPropertyElementMain};

static constexpr ObjectAddr kInputStreamConfig{kAudioDevicePropertyStreamConfiguration,
                                               kAudioDevicePropertyScopeInput,
                                               kAudioObjectPropertyElementMain};

static constexpr ObjectAddr kOutStreamFormat{kAudioDevicePropertyStreamFormat,
                                             kAudioDevicePropertyScopeOutput,
                                             kAudioObjectPropertyElementMain};

static constexpr ObjectAddr kInputStreamFormat{kAudioDevicePropertyStreamFormat,
                                               kAudioDevicePropertyScopeInput,
                                               kAudioObjectPropertyElementMain};

static constexpr ObjectAddr kNominalSampleRateRange{kAudioDevicePropertyAvailableNominalSampleRates,
                                                    kAudioObjectPropertyScopeGlobal,
                                                    kAudioObjectPropertyElementMain};

static constexpr AudioFormatID kFormatPCM = kAudioFormatLinearPCM;

static constexpr AudioFormatFlags kFloat = kAudioFormatFlagIsFloat;
static constexpr AudioFormatFlags kPacked = kAudioFormatFlagIsPacked;
static constexpr UnitPropertyID kUnitStreamFormat = kAudioUnitProperty_StreamFormat;
static constexpr UnitScope kUnitScopeInput = kAudioUnitScope_Input;
static constexpr UnitScope kUnitScopeOutput = kAudioUnitScope_Output;

inline UnitPtr wrapUnit(UnitHandle u) {
    return UnitPtr{static_cast<void*>(u), [](void* p) noexcept {
                       auto unit = static_cast<UnitHandle>(p);
                       if (!unit)
                           return;

                       (void)::AudioOutputUnitStop(unit);
                       (void)::AudioUnitUninitialize(unit);
                       (void)::AudioComponentInstanceDispose(unit);
                   }};
}

inline UnitHandle toRaw(const UnitPtr& u) {
    return static_cast<UnitHandle>(u.get());
}

CoreStatus getPropertyDataSize(ObjectID id,
                               const ObjectAddr* addr,
                               uint32_t inputSize,
                               const void* data,
                               uint32_t* dataSize);

CoreStatus getPropertyData(ObjectID id,
                           const ObjectAddr* addr,
                           uint32_t inputSize,
                           const void* inputData,
                           uint32_t* ioDataSize,
                           void* outputData);

CoreStatus setPropertyData(ObjectID id,
                           const ObjectAddr* addr,
                           uint32_t inputSize,
                           const void* inputData,
                           uint32_t ioDataSize,
                           void* outputData);

CoreStatus startOutput(UnitHandle unit);

CoreStatus stopOutput(UnitHandle unit);

CoreStatus init(UnitHandle unit);

CoreStatus deInit(UnitHandle unit);

CoreStatus newInstance(Component& component, UnitHandle& instance);

CoreStatus deleteInstance(UnitHandle unit);

Component* findNext(Component* component, const ComponentFormat& format);

CoreStatus setUnitProperty(UnitHandle unit,
                           UnitPropertyID id,
                           UnitScope scope,
                           UnitElement element,
                           void* data,
                           uint32_t size);

}  // namespace CoreAudioFacade
