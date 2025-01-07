# examples/CMakeLists.txt
# Add the example executable(s)

# Gather all example source files
file(GLOB EXAMPLE_SOURCES *.cpp)

# Add an executable for each source file
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE RpiSoundLib)
endforeach()
