set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_DEBUG "-g")

add_executable(RpiSoundTest
    unittest_main.cpp
    unittest_alsa_driver.cpp
)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)
FetchContent_MakeAvailable(googletest)

target_link_libraries(RpiSoundTest PRIVATE
    GTest::gtest
    GTest::gmock
    RpiSoundLib
)

add_test(NAME RpiSoundTests COMMAND RpiSoundTest)
