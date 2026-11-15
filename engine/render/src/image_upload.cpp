#include "engine/render/image_upload.hpp"

#include <algorithm>
#include <cstddef>

namespace engine::render {

std::vector<std::uint8_t> flip_rows(const assets::Image& image) {
    if (image.width <= 0 || image.height <= 0) {
        return {};
    }
    const std::size_t row_bytes =
        static_cast<std::size_t>(image.width) * static_cast<std::size_t>(assets::Image::channels);
    const auto rows = static_cast<std::size_t>(image.height);
    if (image.pixels.size() < row_bytes * rows) {
        return {};
    }
    std::vector<std::uint8_t> out(row_bytes * rows);
    for (std::size_t y = 0; y < rows; ++y) {
        const auto src = image.pixels.begin() + static_cast<std::ptrdiff_t>(y * row_bytes);
        std::copy(src, src + static_cast<std::ptrdiff_t>(row_bytes),
                  out.begin() + static_cast<std::ptrdiff_t>((rows - 1 - y) * row_bytes));
    }
    return out;
}

}  // namespace engine::render
