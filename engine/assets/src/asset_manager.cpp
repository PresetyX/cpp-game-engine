#include "engine/assets/asset_manager.hpp"

namespace engine::assets {

LoadResult<Loaded<Image>> AssetLoader<Image>::load(const std::filesystem::path& root,
                                                   const std::string& key) {
    const std::filesystem::path path = root / key;
    auto image = load_image(path);
    if (!image) {
        return LoadResult<Loaded<Image>>::failure(image.error());
    }
    return Loaded<Image>{std::move(*image), {path}};
}

LoadResult<Loaded<Mesh>> AssetLoader<Mesh>::load(const std::filesystem::path& root,
                                                 const std::string& key) {
    const std::filesystem::path path = root / key;
    auto mesh = load_obj(path);
    if (!mesh) {
        return LoadResult<Loaded<Mesh>>::failure(mesh.error());
    }
    return Loaded<Mesh>{std::move(*mesh), {path}};
}

LoadResult<Loaded<ShaderSource>> AssetLoader<ShaderSource>::load(
    const std::filesystem::path& root, const std::string& key) {
    const std::filesystem::path vert = root / (key + ".vert");
    const std::filesystem::path frag = root / (key + ".frag");
    auto source = load_shader_source(vert, frag);
    if (!source) {
        return LoadResult<Loaded<ShaderSource>>::failure(source.error());
    }
    return Loaded<ShaderSource>{std::move(*source), {vert, frag}};
}

}  // namespace engine::assets
