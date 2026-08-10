cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_FOR_AARCH64 "Cross compile for aarch64" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_CPPCHECK "Run CPP check" OFF)
option(ENABLE_CLANG_FORMAT "Run Clang format" OFF)

if(APPLE)
    set(USE_COREAUDIO ON)
    add_compile_definitions(USE_COREAUDIO)
else()
    set(USE_COREAUDIO OFF)
endif()

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

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        10.1.1
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG        v3.2
)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.3
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG        v6.1.9
)
FetchContent_MakeAvailable(ftxui)

if(USE_COREAUDIO)
    set(AUDIO_BACKEND_SRC
        ${CMAKE_SOURCE_DIR}/src/core_audio/core_audio_device_enumerator.cpp
        ${CMAKE_SOURCE_DIR}/src/core_audio/core_audio_driver.cpp
        ${CMAKE_SOURCE_DIR}/src/core_audio/core_audio_facade.cpp
        ${CMAKE_SOURCE_DIR}/src/core_audio/core_audio_utils.cpp
    )
    set(AUDIO_T_COMPILE_DEFINITION RPISOUND_AUDIO_T_FLOAT)
    set(AUDIO_BACKEND_LIBS
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreAudio"
    )
else()
    FetchContent_Declare(
        tinyalsa
        GIT_REPOSITORY https://github.com/tinyalsa/tinyalsa.git
        GIT_TAG        v2.0.0
    )
    FetchContent_MakeAvailable(tinyalsa)
    set(AUDIO_BACKEND_SRC
        ${CMAKE_SOURCE_DIR}/src/alsa/alsa_device_enumerator.cpp
        ${CMAKE_SOURCE_DIR}/src/alsa/alsa_driver.cpp
        ${CMAKE_SOURCE_DIR}/src/alsa/alsa_facade.cpp
    )
    set(AUDIO_T_COMPILE_DEFINITION RPISOUND_AUDIO_T_INT16)
    set(AUDIO_BACKEND_LIBS
        tinyalsa
    )
endif()

# Add library sources
add_library(RpiSoundLib STATIC
    ${AUDIO_BACKEND_SRC}
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/audio_device_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/audio_device.cpp
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/audio_engine.cpp
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/pcm_loader.cpp
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/sound_manager.cpp
)

target_include_directories(RpiSoundLib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(RpiSoundLib PUBLIC
    ${AUDIO_T_COMPILE_DEFINITION}
)

target_link_libraries(RpiSoundLib
    PRIVATE
        pthread
    PUBLIC
        spdlog::spdlog_header_only
)
if(USE_COREAUDIO)
target_link_libraries(RpiSoundLib
    PUBLIC
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreAudio"
)
else()
target_link_libraries(RpiSoundLib
    PUBLIC
        tinyalsa
)
endif()

# Add the source files for standalone executable
add_executable(RpiSound
    src/rpi_sound.cpp
)
target_include_directories(RpiSound PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(RpiSound
    PRIVATE
        RpiSoundLib
        argparse::argparse
        ftxui::screen
        ftxui::component
)

# Determine audio format based on compile definition
if(AUDIO_T_COMPILE_DEFINITION STREQUAL "RPISOUND_AUDIO_T_FLOAT")
    set(AUDIO_FORMAT "F32LE")
else()
    set(AUDIO_FORMAT "S16LE")
endif()

# Create sound database and copy PCM files to binary directory
add_custom_command(TARGET RpiSound POST_BUILD
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/create_sound_db.py
        --path ${CMAKE_SOURCE_DIR}/sound
        --format ${AUDIO_FORMAT}
        --dest $<TARGET_FILE_DIR:RpiSound>/sound/demo
    COMMENT "Creating sound database (${AUDIO_FORMAT}) and copying PCM files"
)

if(BUILD_TESTS)
    add_subdirectory(tests)
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
