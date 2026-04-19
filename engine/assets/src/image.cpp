#include "engine/assets/image.hpp"

#include <algorithm>
#include <limits>
#include <string>

#include "engine/assets/file_io.hpp"
#include "stb_image.h"

namespace engine::assets {

LoadResult<Image> decode_image(const std::uint8_t* data, std::size_t size,
                               const ImageOptions& options) {
    if (data == nullptr || size == 0 ||
        size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return LoadResult<Image>::failure("image: empty or too large input");
    }
    int w = 0;
    int h = 0;
    int original_channels = 0;
    stbi_uc* decoded = stbi_load_from_memory(data, static_cast<int>(size), &w, &h,
                                             &original_channels, Image::channels);
    if (decoded == nullptr) {
        const char* reason = stbi_failure_reason();
        return LoadResult<Image>::failure(std::string("image: ") +
                                          (reason != nullptr ? reason : "decode failed"));
    }
    Image image;
    image.width = w;
    image.height = h;
    const std::size_t row = static_cast<std::size_t>(w) * Image::channels;
    image.pixels.assign(decoded, decoded + row * static_cast<std::size_t>(h));
    stbi_image_free(decoded);

    if (options.flip_vertical) {
        for (int y = 0; y < h / 2; ++y) {
            auto top = image.pixels.begin() + static_cast<std::ptrdiff_t>(row * y);
            auto bottom = image.pixels.begin() + static_cast<std::ptrdiff_t>(row * (h - 1 - y));
            std::swap_ranges(top, top + static_cast<std::ptrdiff_t>(row), bottom);
        }
    }
    return image;
}

LoadResult<Image> load_image(const std::filesystem::path& path, const ImageOptions& options) {
    const auto bytes = read_binary_file(path);
    if (!bytes) {
        return LoadResult<Image>::failure(bytes.error());
    }
    auto image = decode_image(bytes->data(), bytes->size(), options);
    if (!image) {
        return LoadResult<Image>::failure(path.generic_string() + ": " + image.error());
    }
    return image;
}

}  // namespace engine::assets
