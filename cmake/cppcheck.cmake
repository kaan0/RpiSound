# This script checks the source code for potential issues using cppcheck.
# It requires cppcheck to be installed and checks all source files in the project.
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
    add_custom_target(cppcheck
        COMMAND cppcheck --enable=all
                         --std=c++20
                         --language=c++
                         --suppress=missingIncludeSystem
                         --error-exitcode=1
                         --project=${CMAKE_BINARY_DIR}/compile_commands.json
                         -I/usr/include/c++/12
                         -I/usr/include/x86_64-linux-gnu/c++/12
                         -I/usr/include/c++/12/backward
                         -I/usr/lib/gcc/x86_64-linux-gnu/12/include
                         -I/usr/local/include
                         -I/usr/include/x86_64-linux-gnu
                         -I/usr/include
        COMMENT "Running cppcheck..."
        VERBATIM
    )
    # Add cppcheck to the default build
    add_custom_target(run_cppcheck ALL DEPENDS cppcheck)
else()
    message(WARNING "Cppcheck not found, skipping static analysis step.")
endif()
