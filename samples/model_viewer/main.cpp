// Model viewer: loads one OBJ through the AssetManager and draws it with the Phong renderer.
//
//   model_viewer [path/to/model.obj]      (default: the bundled assets/models/torus.obj)
//
//   left mouse drag  orbit        mouse wheel  zoom        R  frame the model again
//   Esc              quit
//
// Hot reload: edit assets/shaders/phong.frag (or phong.vert, or the .obj itself) while it runs;
// it is reloaded within ~1 s. A shader that does not compile is logged with file and line and
// the previous one keeps being used.

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "engine/assets/asset_manager.hpp"
#include "engine/core/log.hpp"
#include "engine/ecs/registry.hpp"
#include "engine/math/transform.hpp"
#include "engine/render/camera.hpp"
#include "engine/render/components.hpp"
#include "engine/render/renderer.hpp"
#include "engine/render/window.hpp"
#include "orbit_controls.hpp"

#ifndef ENGINE_SAMPLE_ASSET_DIR
#error "ENGINE_SAMPLE_ASSET_DIR must point to samples/assets"
#endif

using namespace engine;

namespace {

struct Bounds {
    glm::vec3 center{0.0f};
    float radius = 1.0f;
};

Bounds bounds_of(const assets::Mesh& mesh) {
    if (mesh.vertices.empty()) {
        return {};
    }
    glm::vec3 lo = mesh.vertices.front().position;
    glm::vec3 hi = lo;
    for (const auto& v : mesh.vertices) {
        lo = glm::min(lo, v.position);
        hi = glm::max(hi, v.position);
    }
    const glm::vec3 center = (lo + hi) * 0.5f;
    return {center, std::max(glm::length(hi - center), 1e-3f)};
}

void frame(render::OrbitCamera& camera, const render::PerspectiveCamera& lens, const Bounds& b) {
    camera.target = b.center;
    // Distance at which a sphere of radius b.radius fits the vertical field of view.
    camera.distance = b.radius / std::sin(lens.fov_y * 0.5f) * 1.1f;
    camera.yaw = 0.6f;
    camera.pitch = 0.35f;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 2) {
        std::fprintf(stderr, "usage: %s [model.obj]\n", argv[0]);
        return EXIT_FAILURE;
    }
    const std::filesystem::path asset_dir(ENGINE_SAMPLE_ASSET_DIR);
    const std::filesystem::path model_path =
        std::filesystem::absolute(argc == 2 ? std::filesystem::path(argv[1])
                                            : asset_dir / "models" / "torus.obj");

    // Declaration order is destruction order in reverse: renderer (GL objects), then window
    // (GL context), then assets.
    assets::AssetManager assets{asset_dir};

    // An absolute key is read as is (root / key is the key), so the model can be anywhere.
    auto model = assets.load<assets::Mesh>(model_path.generic_string());
    if (!model) {
        log_error("model: ", model.error());
        return EXIT_FAILURE;
    }

    render::WindowConfig config;
    config.title = "engine - model viewer - " + model_path.filename().string();
    auto window = render::Window::create(config);
    if (!window) {
        log_error(window.error());
        return EXIT_FAILURE;
    }
    auto renderer = render::Renderer::create(assets, "shaders/phong");
    if (!renderer) {
        log_error("renderer: \n", renderer.error());
        return EXIT_FAILURE;
    }

    Registry registry;
    const Entity object = registry.create();
    registry.add<math::Transform>(object, math::Transform{});
    render::MeshRenderer mr;
    mr.mesh = model.value();
    mr.material.diffuse = {0.75f, 0.78f, 0.85f};
    mr.material.specular = {0.5f, 0.5f, 0.5f};
    mr.material.shininess = 48.0f;
    registry.add<render::MeshRenderer>(object, mr);

    const Entity sun = registry.create();
    render::DirectionalLight dl;
    dl.direction = {-0.5f, -1.0f, -0.35f};
    dl.color = {1.0f, 0.96f, 0.9f};
    dl.intensity = 0.8f;
    registry.add<render::DirectionalLight>(sun, dl);

    const glm::vec3 colors[2] = {{1.0f, 0.35f, 0.25f}, {0.25f, 0.5f, 1.0f}};
    Bounds bounds = bounds_of(*assets.get(model.value()));
    const float side = bounds.radius * 2.5f;
    const glm::vec3 positions[2] = {bounds.center + glm::vec3(-side, side * 0.5f, side * 0.5f),
                                    bounds.center + glm::vec3(side, side * 0.3f, -side * 0.5f)};
    for (int i = 0; i < 2; ++i) {
        const Entity e = registry.create();
        render::PointLight pl;
        pl.color = colors[i];
        pl.intensity = 1.5f;
        pl.range = side * 4.0f;
        registry.add<render::PointLight>(e, pl);
        math::Transform t;
        t.position = positions[i];
        registry.add<math::Transform>(e, t);
    }

    render::PerspectiveCamera lens;
    render::OrbitCamera camera;
    frame(camera, lens, bounds);

    sample::OrbitControls orbit(**window);
    sample::KeyPress reframe_key(GLFW_KEY_R);
    std::uint32_t shown_version = 0;
    double next_reload = (*window)->time() + 1.0;

    while (!(*window)->should_close()) {
        (*window)->poll_events();
        if ((*window)->key_down(GLFW_KEY_ESCAPE)) {
            (*window)->request_close();
        }
        orbit.update(**window, camera);
        if (reframe_key.pressed(**window)) {
            frame(camera, lens, bounds);
        }

        const double now = (*window)->time();
        if (now >= next_reload) {
            assets.reload_changed();
            next_reload = now + 1.0;
        }
        // A reloaded model changes the title (triangle count); the camera is left where it is.
        const std::uint32_t version = assets.version(model.value());
        if (version != shown_version) {
            shown_version = version;
            bounds = bounds_of(*assets.get(model.value()));
            const auto triangles = assets.get(model.value())->indices.size() / 3;
            (*window)->set_title("engine - model viewer - " + model_path.filename().string() +
                                 " | " + std::to_string(triangles) + " triangles");
        }

        (*renderer)->render(registry, camera, lens, 1.0f, (*window)->framebuffer_size());
        (*window)->swap_buffers();
    }
    return EXIT_SUCCESS;
}
