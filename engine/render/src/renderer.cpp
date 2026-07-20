#include "engine/render/renderer.hpp"

#include <glad/gl.h>

#include <array>

#include "engine/core/log.hpp"
#include "engine/render/lights.hpp"

namespace engine::render {

assets::LoadResult<std::unique_ptr<Renderer>> Renderer::create(assets::AssetManager& assets,
                                                               const std::string& shader_key) {
    using Result = assets::LoadResult<std::unique_ptr<Renderer>>;
    const auto handle = assets.load<assets::ShaderSource>(shader_key);
    if (!handle) {
        return Result::failure(handle.error());
    }
    const std::string label = (assets.root() / shader_key).generic_string();
    auto program = ShaderProgram::create(*assets.get(handle.value()), label);
    if (!program) {
        assets.release(handle.value());
        return Result::failure(program.error());
    }
    const std::uint32_t version = assets.version(handle.value());
    return std::unique_ptr<Renderer>(
        new Renderer(assets, handle.value(), label, std::move(*program), version));
}

Renderer::Renderer(assets::AssetManager& assets, assets::Handle<assets::ShaderSource> shader_handle,
                   std::string label, ShaderProgram program, std::uint32_t shader_version)
    : assets_(assets),
      shader_handle_(shader_handle),
      label_(std::move(label)),
      program_(std::move(program)),
      shader_version_(shader_version),
      white_(GpuTexture::solid(255, 255, 255, 255)) {}

Renderer::~Renderer() {
    assets_.release(shader_handle_);
}

void Renderer::sync_shader() {
    const std::uint32_t version = assets_.version(shader_handle_);
    if (version == shader_version_ || version == 0) {
        return;
    }
    shader_version_ = version;  // remembered even on failure: do not retry every frame
    const assets::ShaderSource* source = assets_.get(shader_handle_);
    if (source == nullptr) {
        return;
    }
    auto program = ShaderProgram::create(*source, label_);
    if (!program) {
        log_error("shader reload failed, keeping the previous program:\n", program.error());
        return;
    }
    program_ = std::move(*program);
    log_info("shader reloaded: ", label_);
}

const GpuMesh* Renderer::gpu_mesh(assets::Handle<assets::Mesh> handle) {
    const std::uint32_t version = assets_.version(handle);
    if (meshes_.needs_upload(handle, version)) {
        const assets::Mesh* cpu = assets_.get(handle);
        if (cpu == nullptr) {
            return nullptr;
        }
        meshes_.store(handle, version, GpuMesh(*cpu));
    }
    return meshes_.find(handle);
}

const GpuTexture* Renderer::gpu_texture(assets::Handle<assets::Image> handle) {
    if (!handle.valid()) {
        return &white_;
    }
    const std::uint32_t version = assets_.version(handle);
    if (textures_.needs_upload(handle, version)) {
        const assets::Image* cpu = assets_.get(handle);
        if (cpu == nullptr) {
            return &white_;
        }
        textures_.store(handle, version, GpuTexture(*cpu));
    }
    const GpuTexture* tex = textures_.find(handle);
    return tex != nullptr ? tex : &white_;
}

void Renderer::render(const Registry& registry, const glm::mat4& view, const glm::vec3& eye,
                      const PerspectiveCamera& lens, float alpha, FramebufferSize framebuffer) {
    last_draw_count_ = 0;
    const auto projection = lens.projection(framebuffer.width, framebuffer.height);
    if (!projection) {
        return;  // minimized: nothing to draw into
    }

    sync_shader();
    meshes_.collect_stale([&](assets::Handle<assets::Mesh> h) { return assets_.version(h); });
    textures_.collect_stale([&](assets::Handle<assets::Image> h) { return assets_.version(h); });

    glViewport(0, 0, framebuffer.width, framebuffer.height);
    glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    collect_draw_items(registry, alpha, items_);
    const LightBlock lights = pack_lights(registry, eye, alpha);

    program_.use();
    program_.set_mat4("uView", view);
    program_.set_mat4("uProjection", *projection);
    program_.set_vec3("uViewPos", eye);
    program_.set_vec3("uLightDirection", lights.direction);
    program_.set_vec3("uLightColor", lights.directional_color);
    program_.set_vec3("uAmbient", lights.ambient);
    program_.set_int("uPointCount", static_cast<int>(lights.point_count));
    if (lights.point_count > 0) {
        std::array<glm::vec3, kMaxPointLights> pos{};
        std::array<glm::vec3, kMaxPointLights> color{};
        std::array<float, kMaxPointLights> range{};
        for (std::size_t i = 0; i < lights.point_count; ++i) {
            pos[i] = lights.points[i].position;
            color[i] = lights.points[i].color;
            range[i] = lights.points[i].range;
        }
        const int n = static_cast<int>(lights.point_count);
        program_.set_vec3_array("uPointPosition[0]", pos.data(), n);
        program_.set_vec3_array("uPointColor[0]", color.data(), n);
        program_.set_float_array("uPointRange[0]", range.data(), n);
    }
    program_.set_int("uTexture", 0);

    for (const DrawItem& item : items_) {
        const GpuMesh* mesh = gpu_mesh(item.mesh);
        if (mesh == nullptr) {
            continue;  // asset released while still referenced by a component
        }
        gpu_texture(item.texture)->bind(0);
        program_.set_mat4("uModel", item.model);
        program_.set_mat3("uNormalMatrix", item.normal_matrix);
        program_.set_vec3("uDiffuse", item.material.diffuse);
        program_.set_vec3("uSpecular", item.material.specular);
        program_.set_float("uShininess", item.material.shininess);
        mesh->draw();
        ++last_draw_count_;
    }
    glBindVertexArray(0);
    check_gl_error("Renderer::render");
}

}  // namespace engine::render
