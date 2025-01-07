cmake_minimum_required(VERSION 3.10)

# Project details
project(RpiSound)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_FOR_AARCH64 "Cross compile for RaspberryPi" OFF)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_CPPCHECK "Run CPP check" OFF)

if(BUILD_FOR_AARCH64)
    set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc-12)
    set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++-12)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/toolchain-aarch64.cmake")
else()
    set(CMAKE_C_COMPILER /usr/bin/gcc)
    set(CMAKE_CXX_COMPILER /usr/bin/g++)
endif()

# Build TinyALSA as an external project
include(ExternalProject )
ExternalProject_Add(tinyalsa_build
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/tinyalsa
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/tinyalsa
               -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
               -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/tinyalsa/lib/libtinyalsa.a
)

# Define a target for TinyALSA
add_library(tinyalsa STATIC IMPORTED)
set_target_properties(tinyalsa PROPERTIES
    IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/tinyalsa/lib/libtinyalsa.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/tinyalsa/include
)

# Ensure the TinyALSA target is built before your library
add_dependencies(tinyalsa tinyalsa_build)

# Add the source files
add_executable(RpiSound src/rpi_sound.cpp)
target_include_directories(RpiSound PRIVATE ${CMAKE_BINARY_DIR}/tinyalsa/include)

# Add library sources
add_library(RpiSoundLib
    src/audio_utils.cpp
    src/pcm_player.cpp
    src/wav_parser.cpp
    src/rpi_sound.cpp
)

target_include_directories(RpiSoundLib PUBLIC
    third_party/tinyalsa/include
    include
    include/rpi_sound
)

# Use TinyALSA in your targets
target_include_directories(RpiSoundLib PRIVATE ${CMAKE_BINARY_DIR}/tinyalsa/include)
target_link_libraries(RpiSoundLib PRIVATE ${TinyALSA_LIBRARY})

if(BUILD_TESTS)
    # Add tests
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    # Add examples
    add_subdirectory(examples)
endif()

if(ENABLE_CPPCHECK)
    # Add cppcheck as a custom target
    find_program(CPPCHECK cppcheck)
endif()

if(CPPCHECK)
    add_custom_target(cppcheck
        COMMAND cppcheck --enable=all
                         --std=c++20
                         --project=${CMAKE_BINARY_DIR}/compile_commands.json
                         -I/usr/include/c++/14
                         -I/usr/include/x86_64-linux-gnu/c++/14
                         -I/usr/include/c++/14/backward
                         -I/usr/lib/gcc/x86_64-linux-gnu/14/include
                         -I/usr/local/include
                         -I/usr/include/x86_64-linux-gnu
                         -I/usr/include
        COMMENT "Running cppcheck..."
        VERBATIM
    )
    # Add cppcheck to the default build
    add_custom_target(run_cppcheck ALL DEPENDS cppcheck)
endif()
