cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_FOR_AARCH64 "Cross compile for aarch64" OFF)
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

include(FetchContent)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        10.1.1
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
  tinyalsa
  GIT_REPOSITORY https://github.com/tinyalsa/tinyalsa.git
  GIT_TAG        v2.0.0
)
FetchContent_MakeAvailable(tinyalsa)

# Add library sources
add_library(RpiSoundLib
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/alsa_driver.cpp
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/audio_device_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/audio_device.cpp
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/pcm_loader.cpp
    ${CMAKE_SOURCE_DIR}/src/rpi_sound/sound_manager.cpp
)

target_include_directories(RpiSoundLib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(RpiSoundLib
    PRIVATE
        pthread
    PUBLIC
        fmt
        tinyalsa)

# Add the source files for standalone executable
add_executable(RpiSound src/rpi_sound.cpp)
target_include_directories(RpiSound PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(RpiSound PRIVATE RpiSoundLib)

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
