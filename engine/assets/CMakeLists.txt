# stb_image implementation: third-party code, compiled without the engine warning policy.
add_library(engine_stb_image STATIC src/stb_image_impl.cpp)
target_include_directories(engine_stb_image SYSTEM PUBLIC "${STB_IMAGE_DIR}")
target_compile_definitions(engine_stb_image PRIVATE STBI_NO_STDIO)

add_library(engine_assets STATIC
    src/asset_manager.cpp
    src/file_io.cpp
    src/image.cpp
    src/obj.cpp
    src/shader_source.cpp)
add_library(engine::assets ALIAS engine_assets)

target_include_directories(engine_assets PUBLIC include)
target_link_libraries(engine_assets PUBLIC engine::core engine::math PRIVATE engine_stb_image)
engine_set_warnings(engine_assets)
