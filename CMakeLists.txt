cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

option(BUILD_FOR_AARCH64 "Cross compile for aarch64" ON)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_CPPCHECK "Run CPP check" OFF)
option(ENABLE_CLANG_FORMAT "Run Clang format" OFF)

if(BUILD_FOR_AARCH64)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/toolchain-aarch64.cmake")
    install(CODE "
    execute_process(
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/sync-built-files.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/
    )
")
else()
    set(CMAKE_C_COMPILER /usr/bin/gcc)
    set(CMAKE_CXX_COMPILER /usr/bin/g++)
endif()

# Project details
project(RpiSound)

# Build TinyALSA as an external project
include(ExternalProject)
ExternalProject_Add(tinyalsa_build
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/tinyalsa
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/tinyalsa
               -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
               -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
               -DBUILD_SHARED_LIBS=OFF
    BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/tinyalsa/lib/libtinyalsa.a
)

# Define a target for TinyALSA
add_library(tinyalsa STATIC IMPORTED)
set_target_properties(tinyalsa PROPERTIES
    IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/tinyalsa/lib/libtinyalsa.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/third_party/tinyalsa/include
)
add_dependencies(tinyalsa tinyalsa_build)

# Add library sources
add_library(RpiSoundLib
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/alsa_driver.cpp
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/audio_device_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/audio_device.cpp
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/pcm_loader.cpp
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/sound_manager.cpp
)

target_include_directories(RpiSoundLib PUBLIC
    third_party/tinyalsa/include
    include
    include/rpi_sound
)

target_include_directories(RpiSoundLib PRIVATE ${CMAKE_BINARY_DIR}/tinyalsa/include)
target_link_libraries(RpiSoundLib PRIVATE tinyalsa pthread)

# Add the source files for standalone executable
add_executable(RpiSound src/rpi_sound.cpp)
target_include_directories(RpiSound PRIVATE
    ${CMAKE_BINARY_DIR}/tinyalsa/include
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(RpiSound PRIVATE RpiSoundLib)

if(BUILD_TESTS)
    enable_testing()
    if(NOT BUILD_FOR_AARCH64)
        add_subdirectory(tests)
    endif()
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(ENABLE_CLANG_FORMAT)
    include(cmake/clang-format.cmake)
endif()

if(ENABLE_CPPCHECK)
    include(cmake/cppcheck.cmake)
endif()
