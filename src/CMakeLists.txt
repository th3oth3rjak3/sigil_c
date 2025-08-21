cmake_minimum_required(VERSION 3.15)

# Force Clang
set(CMAKE_C_COMPILER clang)

project(sigil VERSION 0.0.1 LANGUAGES C)

# Try to replicate Meson's simpler approach
if(WIN32)
    # Use different runtime library approach
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    # OR try this instead:
    # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    main.c
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
add_executable(sigil ${SOURCES})

# Include directories
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

# Try AddressSanitizer with simpler flags
target_compile_options(sigil PRIVATE 
    -fsanitize=address 
    -g
    -fno-omit-frame-pointer
)

target_link_options(sigil PRIVATE 
    -fsanitize=address
)

if(WIN32)
    target_compile_definitions(sigil PRIVATE _CRT_SECURE_NO_WARNINGS)
    # 8MB stack on Windows
    target_link_options(sigil PRIVATE -Xlinker /STACK:8388608)
    message(STATUS "Windows: Set stack to 8MB")
else()
    # 8MB stack on Linux/Mac
    target_link_options(sigil PRIVATE -Wl,-z,stack-size=8388608)
    message(STATUS "Unix: Set stack to 8MB")
endif()

message(STATUS "Building with AddressSanitizer (Meson-style)")