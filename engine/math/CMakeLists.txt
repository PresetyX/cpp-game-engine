add_library(engine_math INTERFACE)
add_library(engine::math ALIAS engine_math)

target_include_directories(engine_math INTERFACE include)
target_link_libraries(engine_math INTERFACE glm::glm)
target_compile_features(engine_math INTERFACE cxx_std_17)
