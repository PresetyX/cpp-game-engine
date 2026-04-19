#include "engine/assets/shader_source.hpp"

#include "engine/assets/file_io.hpp"

namespace engine::assets {
namespace {

LoadResult<std::string> read_stage(const std::filesystem::path& path) {
    auto text = read_text_file(path);
    if (!text) {
        return text;
    }
    std::string normalized = normalize_text(std::move(*text));
    if (normalized.find_first_not_of(" \t\n") == std::string::npos) {
        return LoadResult<std::string>::failure("shader file is empty: " + path.generic_string());
    }
    return normalized;
}

}  // namespace

LoadResult<ShaderSource> load_shader_source(const std::filesystem::path& vertex_path,
                                            const std::filesystem::path& fragment_path) {
    auto vertex = read_stage(vertex_path);
    if (!vertex) {
        return LoadResult<ShaderSource>::failure(vertex.error());
    }
    auto fragment = read_stage(fragment_path);
    if (!fragment) {
        return LoadResult<ShaderSource>::failure(fragment.error());
    }
    return ShaderSource{std::move(*vertex), std::move(*fragment)};
}

}  // namespace engine::assets
