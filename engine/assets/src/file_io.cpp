#include "engine/assets/file_io.hpp"

#include <fstream>
#include <iterator>

namespace engine::assets {

LoadResult<std::vector<std::uint8_t>> read_binary_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return LoadResult<std::vector<std::uint8_t>>::failure("cannot open file: " +
                                                              path.generic_string());
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
    if (file.bad()) {
        return LoadResult<std::vector<std::uint8_t>>::failure("read error: " +
                                                              path.generic_string());
    }
    return bytes;
}

LoadResult<std::string> read_text_file(const std::filesystem::path& path) {
    const auto bytes = read_binary_file(path);
    if (!bytes) {
        return LoadResult<std::string>::failure(bytes.error());
    }
    return std::string(bytes->begin(), bytes->end());
}

std::string normalize_text(std::string text) {
    std::size_t start = 0;
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        start = 3;
    }
    std::string out;
    out.reserve(text.size() - start);
    for (std::size_t i = start; i < text.size(); ++i) {
        if (text[i] == '\r') {
            out.push_back('\n');
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
        } else {
            out.push_back(text[i]);
        }
    }
    return out;
}

}  // namespace engine::assets
