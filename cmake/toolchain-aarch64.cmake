# toolchain-aarch64.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Specify the cross-compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc-12)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++-12)

# Set the sysroot
set(CMAKE_SYSROOT ~/target-sysroot)

SET(CMAKE_C_FLAGS " -mcpu=cortex-a72+crc+crypto -fstack-protector-strong   -Wformat -Wformat-security -Werror=format-security --sysroot=${CMAKE_SYSROOT}")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

# Search paths for the sysroot
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
