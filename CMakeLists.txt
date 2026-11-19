cmake_minimum_required(VERSION 3.20)

project(cpp_game_engine
    VERSION 0.1.0
    DESCRIPTION "C++17 / OpenGL 3.3 game engine"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(ENGINE_BUILD_TESTS "Build unit tests" ON)
option(ENGINE_BUILD_SAMPLES "Build sample applications" OFF)
option(ENGINE_BUILD_GPU_TESTS "Build tests that need an OpenGL context (never run by CI)" OFF)
option(ENGINE_BUILD_BENCHMARKS "Build benchmarks (not run by ctest)" OFF)
option(ENGINE_WARNINGS_AS_ERRORS "Treat warnings as errors in engine targets" ON)
option(ENGINE_SANITIZE "Build everything with AddressSanitizer + UBSan (GCC/Clang only)" OFF)
option(ENGINE_COVERAGE "Build everything with gcov instrumentation (GCC/Clang only)" OFF)

if(ENGINE_SANITIZE OR ENGINE_COVERAGE)
    if(MSVC)
        message(FATAL_ERROR "ENGINE_SANITIZE / ENGINE_COVERAGE need GCC or Clang")
    endif()
endif()
if(ENGINE_SANITIZE)
    # Any sanitizer report aborts the test, so ctest fails.
    add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all
        -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()
if(ENGINE_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Dependencies)

add_subdirectory(engine)

if(ENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ENGINE_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(ENGINE_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()
