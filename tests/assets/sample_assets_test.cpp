// The model bundled with the samples must load, and an absolute key must load through an
// AssetManager whose root is elsewhere (model_viewer relies on both).

#include <gtest/gtest.h>

#include <filesystem>

#include "engine/assets/asset_manager.hpp"

namespace {

using namespace engine::assets;

const std::filesystem::path kSampleDir(ENGINE_SAMPLE_ASSET_DIR);

TEST(SampleAssets, TorusLoadsThroughAnAbsoluteKey) {
    AssetManager manager{std::filesystem::temp_directory_path()};  // root unrelated to the model
    const auto key = (kSampleDir / "models" / "torus.obj").generic_string();
    const auto handle = manager.load<Mesh>(key);
    ASSERT_TRUE(handle) << handle.error();
    const Mesh* mesh = manager.get(handle.value());
    ASSERT_NE(mesh, nullptr);
    EXPECT_EQ(mesh->vertices.size(), 561u);  // 32 x 16 positions + the uv seam (33 x 17)
    EXPECT_EQ(mesh->indices.size(), 1024u * 3u);
}

TEST(SampleAssets, TorusIndicesAreValidAndFacesPointOutward) {
    const auto mesh = load_obj(kSampleDir / "models" / "torus.obj");
    ASSERT_TRUE(mesh) << mesh.error();
    ASSERT_EQ(mesh->indices.size() % 3, 0u);
    for (std::size_t i = 0; i < mesh->indices.size(); i += 3) {
        ASSERT_LT(mesh->indices[i], mesh->vertices.size());
        ASSERT_LT(mesh->indices[i + 1], mesh->vertices.size());
        ASSERT_LT(mesh->indices[i + 2], mesh->vertices.size());
        const auto& a = mesh->vertices[mesh->indices[i]];
        const auto& b = mesh->vertices[mesh->indices[i + 1]];
        const auto& c = mesh->vertices[mesh->indices[i + 2]];
        const glm::vec3 face = glm::cross(b.position - a.position, c.position - a.position);
        // Counter-clockwise winding agrees with the stored vertex normals.
        EXPECT_GT(glm::dot(face, a.normal + b.normal + c.normal), 0.0f) << "triangle " << i / 3;
    }
}

TEST(SampleAssets, MissingAbsoluteModelReportsAnError) {
    AssetManager manager{kSampleDir};
    const auto missing = (kSampleDir / "models" / "does_not_exist.obj").generic_string();
    EXPECT_FALSE(manager.load<Mesh>(missing));
}

}  // namespace
