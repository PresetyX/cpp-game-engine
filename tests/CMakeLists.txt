add_executable(engine_tests
    core/clock_test.cpp
    core/event_bus_test.cpp
    core/fixed_timestep_test.cpp
    core/log_test.cpp
    core/version_test.cpp
    ecs/entity_pool_test.cpp
    ecs/registry_test.cpp
    ecs/sparse_set_test.cpp
    ecs/view_test.cpp
    math/aabb_test.cpp
    math/transform_test.cpp
    physics/integrate_test.cpp)
target_link_libraries(engine_tests PRIVATE engine::core engine::ecs engine::math engine::physics GTest::gtest_main)
engine_set_warnings(engine_tests)

gtest_discover_tests(engine_tests)
