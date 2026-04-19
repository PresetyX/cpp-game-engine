#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

#include "engine/assets/result.hpp"
#include "engine/math/glm.hpp"

namespace engine::assets {

struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f};
};

// Indexed triangle list in CPU memory (counter-clockwise front faces).
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;  // size is a multiple of 3
};

// Wavefront OBJ: `v`, `vt`, `vn` and `f` (formats v, v/vt, v//vn, v/vt/vn; negative indices;
// polygons are triangulated as a fan). Other statements (o, g, s, usemtl, mtllib, ...) are
// ignored. Vertices are de-duplicated by (position, uv, normal) index triple. If a face has no
// normals, smooth normals are computed from the geometry. Missing uv gives (0, 0).
// Errors carry the line number.
LoadResult<Mesh> parse_obj(std::string_view text);

LoadResult<Mesh> load_obj(const std::filesystem::path& path);

}  // namespace engine::assets
