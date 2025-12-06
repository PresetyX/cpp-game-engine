# Benchmarks are not registered with ctest; run the executables by hand.
add_executable(ecs_bench ecs_bench.cpp)
target_link_libraries(ecs_bench PRIVATE engine::ecs)
engine_set_warnings(ecs_bench)
