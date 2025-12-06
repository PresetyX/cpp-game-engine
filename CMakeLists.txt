cmake_minimum_required(VERSION 3.20)

project(cpp_game_engine
    VERSION 0.1.0
    DESCRIPTION "C++17 / OpenGL 3.3 game engine"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(ENGINE_BUILD_TESTS "Build unit tests" ON)
option(ENGINE_BUILD_SAMPLES "Build sample applications" OFF)
option(ENGINE_WARNINGS_AS_ERRORS "Treat warnings as errors in engine targets" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Dependencies)

add_subdirectory(engine)

if(ENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ENGINE_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()
