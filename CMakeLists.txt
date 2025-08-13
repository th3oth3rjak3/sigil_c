cmake_minimum_required(VERSION 3.15)

project(sigil_c C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.c
    # Add more .c files here as you add them
)

add_executable(sigil ${SOURCES})

target_include_directories(sigil
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)
