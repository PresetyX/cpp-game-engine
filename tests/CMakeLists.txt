add_executable(engine_tests core/version_test.cpp)
target_link_libraries(engine_tests PRIVATE engine::core GTest::gtest_main)
engine_set_warnings(engine_tests)

gtest_discover_tests(engine_tests)
