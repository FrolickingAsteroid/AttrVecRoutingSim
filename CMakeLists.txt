cmake_minimum_required(VERSION 3.20)

project(Simulator
        VERSION 0.1.0
        LANGUAGES CXX
)

# ------------------------------------------------------------
# C++ standard
# ------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------
# Build options
# ------------------------------------------------------------

option(SIMULATOR_ENABLE_WARNINGS "Enable compiler warnings" ON)

# ------------------------------------------------------------
# Executable
# ------------------------------------------------------------

add_executable(Simulator
        src/main.cpp
)

# ------------------------------------------------------------
# Include paths
# ------------------------------------------------------------

target_include_directories(Simulator
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ------------------------------------------------------------
# Warnings
# ------------------------------------------------------------

if(SIMULATOR_ENABLE_WARNINGS)
    target_compile_options(Simulator
            PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:
            -Wall
            -Wextra
            #-Wpedantic
            -Wconversion
            -Wshadow
            >
            $<$<CXX_COMPILER_ID:MSVC>:
            /W4
            >
    )
endif()

# ------------------------------------------------------------
# Debug/Release defaults
# ------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()