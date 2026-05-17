add_executable(engine_tests
    assets/asset_manager_test.cpp
    assets/image_test.cpp
    assets/obj_test.cpp
    assets/shader_source_test.cpp
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
    physics/broadphase_test.cpp
    physics/integrate_test.cpp
    physics/narrowphase_test.cpp
    physics/solver_test.cpp
    physics/world_test.cpp
    render/camera_test.cpp
    render/draw_list_test.cpp
    render/shader_log_test.cpp
    render/upload_cache_test.cpp)
target_link_libraries(engine_tests PRIVATE engine::assets engine::core engine::ecs engine::math engine::physics engine::render_core GTest::gtest_main)
target_compile_definitions(engine_tests PRIVATE ENGINE_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/fixtures")
target_include_directories(engine_tests PRIVATE assets)
engine_set_warnings(engine_tests)

gtest_discover_tests(engine_tests)
