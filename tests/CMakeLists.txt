find_package(GTest REQUIRED)

add_executable(RpiSoundTest
    unittest_wav_parse.cpp
)

# target_include_directories(RpiSoundTest PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(RpiSoundTest PRIVATE
    GTest::gtest_main
    RpiSoundLib
)

# include(GoogleTest)
# gtest_discover_tests(RpiSoundTest)
add_test(NAME RpiSoundTests COMMAND RpiSoundTest)
