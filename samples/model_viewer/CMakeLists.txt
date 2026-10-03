# Model viewer: loads an OBJ through the AssetManager and draws it with the Phong renderer.
# Built only with -DENGINE_BUILD_SAMPLES=ON; CI compiles it but never runs it.
add_executable(model_viewer main.cpp)
target_link_libraries(model_viewer PRIVATE engine::render engine::assets engine::ecs
    engine::core engine::math glfw)
target_include_directories(model_viewer PRIVATE ../common)
# The source folder, not a copy, so editing a shader while the sample runs hot reloads it.
target_compile_definitions(model_viewer PRIVATE
    ENGINE_SAMPLE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../assets" GLFW_INCLUDE_NONE)
engine_set_warnings(model_viewer)
