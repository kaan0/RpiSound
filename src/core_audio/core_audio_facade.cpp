#include "core_audio/core_audio_facade.hpp"
#include "core_audio/core_audio_utils.hpp"

CoreAudioFacade::CoreStatus CoreAudioFacade::getPropertyDataSize(CoreAudioFacade::ObjectID id,
                                                                 const CoreAudioFacade::ObjectAddr* addr,
                                                                 uint32_t inputSize,
                                                                 const void* data,
                                                                 uint32_t* dataSize) {
    return AudioObjectGetPropertyDataSize(id, addr, inputSize, data, dataSize);
}

CoreAudioFacade::CoreStatus CoreAudioFacade::getPropertyData(CoreAudioFacade::ObjectID id,
                                                             const CoreAudioFacade::ObjectAddr* addr,
                                                             uint32_t inputSize,
                                                             const void* inputData,
                                                             uint32_t* ioDataSize,
                                                             void* outputData) {
    return AudioObjectGetPropertyData(id, addr, inputSize, inputData, ioDataSize, outputData);
}

CoreAudioFacade::CoreStatus CoreAudioFacade::setPropertyData(CoreAudioFacade::ObjectID id,
                                                             const CoreAudioFacade::ObjectAddr* addr,
                                                             uint32_t inputSize,
                                                             const void* inputData,
                                                             uint32_t ioDataSize,
                                                             void* outputData) {
    return AudioObjectSetPropertyData(id, addr, inputSize, inputData, ioDataSize, outputData);
}

CoreAudioFacade::CoreStatus CoreAudioFacade::startOutput(CoreAudioFacade::UnitHandle unit) {
    return AudioOutputUnitStart(unit);
}

CoreAudioFacade::CoreStatus CoreAudioFacade::stopOutput(CoreAudioFacade::UnitHandle unit) {
    return AudioOutputUnitStop(unit);
}

CoreAudioFacade::CoreStatus CoreAudioFacade::init(CoreAudioFacade::UnitHandle unit) {
    return AudioUnitInitialize(unit);
}

CoreAudioFacade::CoreStatus CoreAudioFacade::deInit(CoreAudioFacade::UnitHandle unit) {
    return AudioUnitUninitialize(unit);
}

CoreAudioFacade::CoreStatus CoreAudioFacade::newInstance(CoreAudioFacade::Component& component,
                                                         CoreAudioFacade::UnitHandle& instance) {
    return AudioComponentInstanceNew(&component, reinterpret_cast<CoreAudioFacade::ComponentInstance**>(&instance));
}

CoreAudioFacade::CoreStatus CoreAudioFacade::deleteInstance(CoreAudioFacade::UnitHandle unit) {
    return AudioComponentInstanceDispose(unit);
}

CoreAudioFacade::Component* CoreAudioFacade::findNext(CoreAudioFacade::Component* component,
                                                      const CoreAudioFacade::ComponentFormat& format) {
    return AudioComponentFindNext(component, &format);
}

CoreAudioFacade::CoreStatus CoreAudioFacade::setUnitProperty(CoreAudioFacade::UnitHandle unit,
                                                             CoreAudioFacade::UnitPropertyID id,
                                                             CoreAudioFacade::UnitScope scope,
                                                             CoreAudioFacade::UnitElement element,
                                                             void* data,
                                                             uint32_t size) {
    return AudioUnitSetProperty(unit, id, scope, element, data, size);
}
