#pragma once

#include <memory>
#include <string>
#include <vector>

#include "engine/assets/asset_manager.hpp"
#include "engine/ecs/registry.hpp"
#include "engine/render/camera.hpp"
#include "engine/render/draw_list.hpp"
#include "engine/render/gl_objects.hpp"
#include "engine/render/upload_cache.hpp"
#include "engine/render/window.hpp"

namespace engine::render {

// Forward Blinn-Phong renderer: one directional light, up to kMaxPointLights point lights, depth
// test, back-face culling. No shadows, no PBR.
//
// The renderer keeps GPU copies of meshes and textures keyed by asset handle and the asset's
// version(): when AssetManager::reload_changed() reloads a file, the next frame re-uploads it.
// The shader program is recompiled the same way; if the new source does not compile the error
// is logged with file and line and the previous program keeps being used.
//
// Needs a current GL context (a Window); destroy it before the Window. `assets` must outlive it.
class Renderer {
public:
    // `shader_key` is the AssetManager key without extension ("shaders/phong").
    static assets::LoadResult<std::unique_ptr<Renderer>> create(assets::AssetManager& assets,
                                                                const std::string& shader_key);
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    glm::vec3 clear_color{0.08f, 0.09f, 0.11f};

    // Draws the scene. `alpha` is FixedTimestep::alpha(): entities with a PreviousTransform are
    // drawn between their previous and current simulation state. Does nothing for an empty
    // framebuffer (minimized window).
    void render(const Registry& registry, const glm::mat4& view, const glm::vec3& eye,
                const PerspectiveCamera& lens, float alpha, FramebufferSize framebuffer);

    void render(const Registry& registry, const OrbitCamera& camera, const PerspectiveCamera& lens,
                float alpha, FramebufferSize framebuffer) {
        render(registry, camera.view(), camera.eye(), lens, alpha, framebuffer);
    }

    // Statistics of the last render() call.
    std::size_t last_draw_count() const { return last_draw_count_; }

private:
    Renderer(assets::AssetManager& assets, assets::Handle<assets::ShaderSource> shader_handle,
             std::string label, ShaderProgram program, std::uint32_t shader_version);

    void sync_shader();
    const GpuMesh* gpu_mesh(assets::Handle<assets::Mesh> handle);
    const GpuTexture* gpu_texture(assets::Handle<assets::Image> handle);

    assets::AssetManager& assets_;
    assets::Handle<assets::ShaderSource> shader_handle_;
    std::string label_;
    ShaderProgram program_;
    std::uint32_t shader_version_;

    UploadCache<assets::Handle<assets::Mesh>, GpuMesh> meshes_;
    UploadCache<assets::Handle<assets::Image>, GpuTexture> textures_;
    GpuTexture white_;
    std::vector<DrawItem> items_;
    std::size_t last_draw_count_ = 0;
};

}  // namespace engine::render
