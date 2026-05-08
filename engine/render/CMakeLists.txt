# engine_render_core: render logic with no OpenGL and no window (camera, shader log parsing,
# upload cache, draw lists, light packing). Linked into engine_tests, so CI covers it.
add_library(engine_render_core STATIC
    src/camera.cpp)
add_library(engine::render_core ALIAS engine_render_core)

target_include_directories(engine_render_core PUBLIC include)
target_link_libraries(engine_render_core PUBLIC engine::assets engine::core engine::ecs engine::math)
engine_set_warnings(engine_render_core)

# engine_render: everything that talks to OpenGL/GLFW. Needs a window, so it is compiled in CI
# but never run there.
add_library(engine_render STATIC
    src/window.cpp)
add_library(engine::render ALIAS engine_render)

target_include_directories(engine_render PUBLIC include)
target_compile_definitions(engine_render PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(engine_render PUBLIC engine::render_core PRIVATE engine_glad glfw)
engine_set_warnings(engine_render)
