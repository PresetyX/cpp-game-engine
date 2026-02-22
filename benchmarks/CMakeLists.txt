# Benchmarks are not registered with ctest; run the executables by hand.
add_executable(ecs_bench ecs_bench.cpp)
target_link_libraries(ecs_bench PRIVATE engine::ecs)
engine_set_warnings(ecs_bench)

add_executable(physics_bench physics_bench.cpp)
target_link_libraries(physics_bench PRIVATE engine::physics)
engine_set_warnings(physics_bench)
