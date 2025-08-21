cmake_minimum_required(VERSION 3.15)
project(sigil VERSION 0.0.1 LANGUAGES C)

# Set C standard and policies
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Library sources
set(LIB_SOURCES
    compiler/compiler.c
    debug/debug.c
    error_handling/error_handler.c
    memory/memory.c
    runtime/bytecode.c
    runtime/vm.c
    scanner/scanner.c
    types/hash_map.c
    types/object.c
    types/value.c
)

# Main executable
add_executable(sigil main.c ${LIB_SOURCES})

# Add include directories to the main target
target_include_directories(sigil PRIVATE
    .
    compiler
    debug
    error_handling
    memory
    runtime
    scanner
    types
)

# Find Unity
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.5.2
)
FetchContent_MakeAvailable(unity)

# Test files
set(TEST_FILES
    compiler/compiler_tests.c
    error_handling/error_handler_tests.c
    memory/memory_tests.c
    runtime/bytecode_tests.c
    runtime/vm_tests.c
    scanner/scanner_tests.c
    types/hash_map_tests.c
    types/object_tests.c
    types/value_tests.c
)

# Enable testing
enable_testing()

# Create and register tests
foreach(test_file ${TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)

    # Create test executable
    add_executable(${test_name} ${LIB_SOURCES} ${test_file})

    # Add include directories to the test target
    target_include_directories(${test_name} PRIVATE
        .
        compiler
        debug
        error_handling
        memory
        runtime
        scanner
        types
    )

    # Link with Unity and add sanitizer flags
    target_link_libraries(${test_name} PRIVATE unity)
    target_compile_options(${test_name} PRIVATE -fsanitize=address)
    target_link_options(${test_name} PRIVATE -fsanitize=address)

    # Register the test
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
