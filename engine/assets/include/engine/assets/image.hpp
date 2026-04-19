#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "engine/assets/result.hpp"

namespace engine::assets {

// Decoded pixels in CPU memory: always 8-bit RGBA, rows from top to bottom, tightly packed.
// Uploading to the GPU is a separate step (the renderer), so no window is needed to load one.
struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;  // width * height * 4 bytes

    static constexpr int channels = 4;

    // Pixel (x, y), origin at the top-left corner. Precondition: inside the image.
    const std::uint8_t* pixel(int x, int y) const {
        return pixels.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                static_cast<std::size_t>(x)) *
                                   channels;
    }
};

struct ImageOptions {
    bool flip_vertical = false;  // OpenGL expects the first row at the bottom
};

// Decodes PNG, JPEG, BMP, TGA, ... (what stb_image supports) from memory.
LoadResult<Image> decode_image(const std::uint8_t* data, std::size_t size,
                               const ImageOptions& options = {});

LoadResult<Image> load_image(const std::filesystem::path& path, const ImageOptions& options = {});

}  // namespace engine::assets
