# Scene sample: physics + Phong renderer. Built only with -DENGINE_BUILD_SAMPLES=ON; CI compiles
# it but never runs it (it needs a window and an OpenGL 3.3 driver).
add_executable(scene_sample main.cpp)
target_link_libraries(scene_sample PRIVATE engine::render engine::physics engine::assets
    engine::ecs engine::core engine::math glfw)
# The source folder, not a copy, so editing a shader while the sample runs hot reloads it.
target_compile_definitions(scene_sample PRIVATE
    ENGINE_SAMPLE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets" GLFW_INCLUDE_NONE)
engine_set_warnings(scene_sample)
