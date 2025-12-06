add_library(engine_ecs STATIC
    src/entity_pool.cpp
    src/registry.cpp)
add_library(engine::ecs ALIAS engine_ecs)

target_include_directories(engine_ecs PUBLIC include)
engine_set_warnings(engine_ecs)
