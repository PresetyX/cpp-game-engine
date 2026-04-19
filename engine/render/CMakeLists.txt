# engine_render: everything that talks to OpenGL/GLFW. Needs a window, so it is compiled in CI
# but never run there. Pure logic (camera, parsing, draw lists) lives in engine_render_core.
add_library(engine_render STATIC
    src/window.cpp)
add_library(engine::render ALIAS engine_render)

target_include_directories(engine_render PUBLIC include)
target_compile_definitions(engine_render PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(engine_render PUBLIC engine::assets engine::core engine::math PRIVATE engine_glad glfw)
engine_set_warnings(engine_render)
