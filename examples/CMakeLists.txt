# Get all source files from the directory
file(GLOB EXAMPLE_SOURCES *.cpp)

# Add an executable for each source file
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE RpiSoundLib)
endforeach()

find_package(Python3 REQUIRED)
set(GENERATED_FILE ${CMAKE_CURRENT_BINARY_DIR}/output.wav)

add_custom_target(
    GenerateWavFile
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/wav_generator.py -o ${GENERATED_FILE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts
    COMMENT "Running wav_generator.py to generate a wav file."
)
# Export target
set_target_properties(GenerateWavFile PROPERTIES EXCLUDE_FROM_ALL OFF)