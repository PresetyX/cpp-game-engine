#pragma once

#include <filesystem>
#include <string>

#include "engine/assets/result.hpp"

namespace engine::assets {

// GLSL source text only: nothing is compiled and OpenGL is not involved here.
struct ShaderSource {
    std::string vertex;
    std::string fragment;
};

// Reads both files as normalized text (see normalize_text). Fails if a file is missing or empty.
LoadResult<ShaderSource> load_shader_source(const std::filesystem::path& vertex_path,
                                            const std::filesystem::path& fragment_path);

}  // namespace engine::assets
