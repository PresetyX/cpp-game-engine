add_library(engine_physics STATIC
    src/broadphase.cpp
    src/integrate.cpp
    src/narrowphase.cpp
    src/solver.cpp)
add_library(engine::physics ALIAS engine_physics)

target_include_directories(engine_physics PUBLIC include)
target_link_libraries(engine_physics PUBLIC engine::ecs engine::math)
engine_set_warnings(engine_physics)
