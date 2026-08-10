#pragma once

#include <memory>
#include <vector>

#include <CoreAudio/CoreAudio.h>

#include "core_audio/core_audio_facade.hpp"
#include "types/audio_device_info.hpp"
#include "types/result.hpp"

namespace CoreAudioUtils {

/**
 * \brief Get the Property size of an AudioObject
 * 
 * \param[in] inObjectID AudioObject ID
 * \param[in] inAddress address of property
 * \return size of the property on success, std:unexpected with error message on failure
 */
types::Result<uint32_t> getPropertySize(CoreAudioFacade::ObjectID inObjectID,
                                        const CoreAudioFacade::ObjectAddr* inAddress);

/**
 * \brief Get the Property of an AudioObject
 * 
 * \param[in] inObjectID AudioObject ID
 * \param[in] inAddress address of property
 * \param[in] dataSize size of the data struct
 * \param[out] data output data
 * \return size of output on success, std:unexpected with error message on failure
 */
types::Result<uint32_t> getProperty(CoreAudioFacade::ObjectID inObjectID,
                                    const CoreAudioFacade::ObjectAddr* inAddress,
                                    uint32_t dataSize,
                                    void* data);

template <typename T>
types::Result<std::vector<T>> getPropertyArray(CoreAudioFacade::ObjectID objectId,
                                               const CoreAudioFacade::ObjectAddr* addr) {
    static_assert(std::is_trivially_copyable_v<T>);

    auto size = getPropertySize(objectId, addr);
    if (!size) {
        return std::unexpected(size.error());
    }

    if (size.value() % sizeof(T) != 0) {
        return std::unexpected("Property size is not a multiple of element size");
    }

    const uint32_t count = size.value() / sizeof(T);
    std::vector<T> data(count);

    auto r = getProperty(objectId, addr, static_cast<uint32_t>(data.size() * sizeof(T)), data.data());
    if (!r) {
        return std::unexpected(r.error());
    }

    return data;
}

types::Result<void> setProperty(CoreAudioFacade::ObjectID inObjectID,
                                const CoreAudioFacade::ObjectAddr* inAddress,
                                uint32_t dataSize,
                                void* data);

types::Result<void> setProperty(CoreAudioFacade::UnitHandle unit,
                                CoreAudioFacade::UnitPropertyID propertyId,
                                CoreAudioFacade::UnitScope scope,
                                uint32_t dataSize,
                                void* data);
/**
 * @brief Get default output device
 *
 * @param outDeviceId Filled with default output AudioDeviceID on success
 * @return none on success, std:unexpected with error message on failure
 */
types::Result<void> getDefaultOutputDevice(CoreAudioFacade::DeviceID& outDeviceId);

/**
 * @brief Get default input device
 *
 * @param inputDeviceId Filled with default input AudioDeviceID on success
 * @return none on success, std:unexpected with error message on failure
 */
types::Result<void> getDefaultInputDevice(CoreAudioFacade::DeviceID& inputDeviceId);

/**
 * @brief Get human-readable device name
 */
types::Result<std::string> getDeviceName(CoreAudioFacade::DeviceID deviceId);

/**
 * @brief Get current stream format for a device direction
 *
 * @param deviceId Device id
 * @param isInput true for input scope, false for output
 * @param outFormat Filled with current AudioStreamBasicDescription
 * @return none on success, std:unexpected with error message on failure
 */
types::Result<void> getDeviceStreamFormat(CoreAudioFacade::DeviceID deviceId,
                                          bool isInput,
                                          CoreAudioFacade::StreamFormat& outFormat);

/**
 * @brief Set stream format for a device direction
 */
types::Result<void> setUnitStreamFormat(const CoreAudioFacade::UnitPtr& unit,
                                        bool isInput,
                                        CoreAudioFacade::StreamFormat& format);

/**
 * @brief Create an output AudioUnit bound to a specific device
 */
types::Result<CoreAudioFacade::UnitPtr> createOutputUnit(const CoreAudioFacade::ComponentFormat& format);

/**
 * @brief Create an output AudioUnit using the default output device
 */
types::Result<CoreAudioFacade::UnitPtr> createDefaultOutputUnit();

types::Result<void> bindUnitToDevice(const CoreAudioFacade::UnitPtr& unit, CoreAudioFacade::DeviceID device);

types::Result<CoreAudioFacade::UnitPtr> createDefaultOutputUnitForDevice(CoreAudioFacade::DeviceID deviceId);

/**
 * @brief Initialize AudioUnit
 */
types::Result<void> initializeUnit(const CoreAudioFacade::UnitPtr& unit);

/**
 * @brief Uninitialize AudioUnit
 */
types::Result<void> uninitializeUnit(const CoreAudioFacade::UnitPtr& unit);

/**
 * @brief Start AudioUnit I/O
 */
types::Result<void> start(const CoreAudioFacade::UnitPtr& unit);

/**
 * @brief Stop AudioUnit I/O
 */
types::Result<void> stop(const CoreAudioFacade::UnitPtr& unit);

/**
 * @brief Dispose AudioUnit
 */
types::Result<void> dispose(const CoreAudioFacade::UnitPtr& unit);

/**
 * @brief Set render callback for output AudioUnit
 *
 * @param unit AudioUnit handle
 * @param callback AURenderCallback function
 * @param userData Pointer passed to callback
 */
types::Result<void> setRenderCallback(const CoreAudioFacade::UnitPtr& unit, AURenderCallback callback, void* userData);

/**
 * @brief Convert frames to bytes using a stream format
 */
uint32_t framesToBytes(const CoreAudioFacade::StreamFormat& format, uint32_t frames);

/**
 * @brief Convert bytes to frames using a stream format
 */
uint32_t bytesToFrames(const CoreAudioFacade::StreamFormat& format, uint32_t bytes);

const types::AudioDeviceInfo::DeviceFormat& getDefaultDeviceFormat();

/**
 * @brief Simple CoreStatus to const char* helper
 */
const char* osStatusToString(CoreAudioFacade::CoreStatus status);

}  // namespace CoreAudioUtils
