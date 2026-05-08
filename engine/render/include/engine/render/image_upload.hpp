#pragma once

#include <cstdint>
#include <vector>

#include "engine/assets/image.hpp"

namespace engine::render {

// Pixels of `image` with the rows in reverse order. The asset module keeps the first row at the
// top (image convention); OpenGL expects the first row uploaded to be the bottom of the texture,
// so the renderer uploads the flipped copy and uv (0,0) stays the bottom-left corner.
std::vector<std::uint8_t> flip_rows(const assets::Image& image);

}  // namespace engine::render
