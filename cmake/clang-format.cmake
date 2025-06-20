# This script checks the formatting of source files using clang-format.
# It requires clang-format to be installed and checks all .cpp and .hpp files

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES src/*.cpp src/*/*.cpp include/*.hpp include/*/*.hpp)
    add_custom_target(clang-format
        COMMAND ${CLANG_FORMAT} -style=file -Werror --dry-run ${ALL_SOURCE_FILES}
        COMMENT "Checking source files formatting with clang-format..."
        VERBATIM
    )
    # Add clang-format to the default build
    add_custom_target(run_clang_format ALL DEPENDS clang-format)
else()
    message(WARNING "Clang format not found, skipping formatting step.")
endif()
