find_package(GTest REQUIRED)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_DEBUG "-g")

add_executable(RpiSoundTest
    unittest_main.cpp
    unittest_wav_parse.cpp
)

# target_include_directories(RpiSoundTest PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(RpiSoundTest PRIVATE
    GTest::gtest
    RpiSoundLib
)

# include(GoogleTest)
# gtest_discover_tests(RpiSoundTest)
add_test(NAME RpiSoundTests COMMAND RpiSoundTest)
