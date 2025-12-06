add_library(engine_core STATIC
    src/clock.cpp
    src/log.cpp
    src/version.cpp)
add_library(engine::core ALIAS engine_core)

target_include_directories(engine_core PUBLIC include)
engine_set_warnings(engine_core)
