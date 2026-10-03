// Physics playground: spheres and boxes fall and collide on a static floor (engine_physics),
// drawn with the Phong renderer.
//
//   left mouse drag  orbit        mouse wheel  zoom        Space  drop a new body
//   R                reset the scene                        Esc    quit
//
// The window title shows the number of bodies and the time of the last physics step (average
// over the last ~0.5 s). Edit assets/shaders/phong.frag (or a mesh/texture) while it runs: it
// is reloaded within ~1 s. The fixed-step simulation runs at 60 Hz; drawing interpolates with
// FixedTimestep::alpha().

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>

#include "engine/assets/asset_manager.hpp"
#include "engine/core/fixed_timestep.hpp"
#include "engine/core/log.hpp"
#include "engine/ecs/registry.hpp"
#include "engine/math/transform.hpp"
#include "engine/physics/components.hpp"
#include "engine/physics/world.hpp"
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

constexpr float kStep = 1.0f / 60.0f;
constexpr std::size_t kMaxBodies = 300;
constexpr int kInitialBodies = 40;

struct Scene {
    Registry registry;  // replaced by reset()
    assets::Handle<assets::Mesh> cube;
    assets::Handle<assets::Mesh> sphere;
    assets::Handle<assets::Image> checker;
    std::size_t bodies = 0;
    std::mt19937 rng{1234};
};

float random_in(std::mt19937& rng, float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(rng);
}

void add_floor(Scene& s) {
    const Entity e = s.registry.create();
    math::Transform t;
    t.position = {0.0f, -0.5f, 0.0f};
    t.scale = {24.0f, 1.0f, 24.0f};
    s.registry.add<math::Transform>(e, t);
    s.registry.add<physics::Collider>(e, physics::Collider::box({12.0f, 0.5f, 12.0f}, 0.6f));
    render::MeshRenderer mr;
    mr.mesh = s.cube;
    mr.texture = s.checker;
    mr.material.diffuse = {0.9f, 0.9f, 0.9f};
    mr.material.specular = {0.1f, 0.1f, 0.1f};
    s.registry.add<render::MeshRenderer>(e, mr);
}

void add_body(Scene& s, bool sphere) {
    if (s.bodies >= kMaxBodies) {
        return;
    }
    ++s.bodies;
    const Entity e = s.registry.create();
    math::Transform t;
    t.position = {random_in(s.rng, -3.0f, 3.0f), random_in(s.rng, 6.0f, 14.0f),
                  random_in(s.rng, -3.0f, 3.0f)};
    render::MeshRenderer mr;
    mr.texture = random_in(s.rng, 0.0f, 1.0f) < 0.5f ? s.checker : assets::Handle<assets::Image>{};
    mr.material.diffuse = {random_in(s.rng, 0.3f, 1.0f), random_in(s.rng, 0.3f, 1.0f),
                           random_in(s.rng, 0.3f, 1.0f)};
    mr.material.specular = {0.6f, 0.6f, 0.6f};
    mr.material.shininess = 48.0f;
    if (sphere) {
        const float radius = random_in(s.rng, 0.3f, 0.6f);
        t.scale = glm::vec3(radius * 2.0f);
        mr.mesh = s.sphere;
        s.registry.add<physics::Collider>(e, physics::Collider::sphere(radius, 0.4f, 0.3f));
    } else {
        const glm::vec3 half(random_in(s.rng, 0.3f, 0.6f));
        t.scale = half * 2.0f;
        mr.mesh = s.cube;
        s.registry.add<physics::Collider>(e, physics::Collider::box(half, 0.5f, 0.1f));
    }
    s.registry.add<math::Transform>(e, t);
    s.registry.add<math::PreviousTransform>(e, math::PreviousTransform{t});
    s.registry.add<physics::RigidBody>(e, physics::RigidBody::with_mass(1.0f));
    s.registry.add<render::MeshRenderer>(e, mr);
}

void add_lights(Scene& s);

// Starts over: new registry, floor, lights and the initial bodies, same random sequence.
void reset(Scene& s) {
    s.registry = Registry{};
    s.bodies = 0;
    s.rng.seed(1234);
    add_floor(s);
    add_lights(s);
    for (int i = 0; i < kInitialBodies; ++i) {
        add_body(s, i % 2 == 0);
    }
}

void add_lights(Scene& s) {
    const Entity sun = s.registry.create();
    render::DirectionalLight dl;
    dl.direction = {-0.5f, -1.0f, -0.35f};
    dl.color = {1.0f, 0.96f, 0.9f};
    dl.intensity = 0.8f;
    s.registry.add<render::DirectionalLight>(sun, dl);

    const glm::vec3 colors[2] = {{1.0f, 0.35f, 0.25f}, {0.25f, 0.5f, 1.0f}};
    const glm::vec3 positions[2] = {{-6.0f, 3.0f, 2.0f}, {6.0f, 3.0f, -2.0f}};
    for (int i = 0; i < 2; ++i) {
        const Entity e = s.registry.create();
        render::PointLight pl;
        pl.color = colors[i];
        pl.intensity = 1.5f;
        pl.range = 16.0f;
        s.registry.add<render::PointLight>(e, pl);
        math::Transform t;
        t.position = positions[i];
        s.registry.add<math::Transform>(e, t);
    }
}

}  // namespace

int main() {
    // Declaration order is destruction order in reverse: renderer (GL objects, releases its
    // shader asset), then window (GL context), then assets.
    assets::AssetManager assets{std::filesystem::path(ENGINE_SAMPLE_ASSET_DIR)};

    render::WindowConfig config;
    config.title = "engine - physics playground";
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

    Scene scene;
    const auto cube = assets.load<assets::Mesh>("meshes/cube.obj");
    const auto sphere = assets.load<assets::Mesh>("meshes/sphere.obj");
    const auto checker = assets.load<assets::Image>("textures/checker.png");
    for (const auto* r : {&cube, &sphere}) {
        if (!*r) {
            log_error("mesh: ", r->error());
            return EXIT_FAILURE;
        }
    }
    if (!checker) {
        log_error("texture: ", checker.error());
        return EXIT_FAILURE;
    }
    scene.cube = cube.value();
    scene.sphere = sphere.value();
    scene.checker = checker.value();

    reset(scene);

    physics::PhysicsWorld physics;
    FixedTimestep timestep(kStep);
    render::PerspectiveCamera lens;
    render::OrbitCamera camera;
    camera.target = {0.0f, 2.0f, 0.0f};
    camera.distance = 22.0f;

    double last_time = (*window)->time();
    double next_reload = last_time + 1.0;
    sample::OrbitControls orbit(**window);
    sample::KeyPress space_key(GLFW_KEY_SPACE);
    sample::KeyPress reset_key(GLFW_KEY_R);
    bool sphere_next = true;
    double step_ms_sum = 0.0;  // physics time since the last title update
    int step_count = 0;
    double next_title = last_time + 0.5;

    while (!(*window)->should_close()) {
        (*window)->poll_events();
        if ((*window)->key_down(GLFW_KEY_ESCAPE)) {
            (*window)->request_close();
        }

        const double now = (*window)->time();
        const double frame_dt = now - last_time;
        last_time = now;

        orbit.update(**window, camera);
        if (space_key.pressed(**window)) {
            add_body(scene, sphere_next);
            sphere_next = !sphere_next;
        }
        if (reset_key.pressed(**window)) {
            reset(scene);
            sphere_next = true;
        }

        // Simulation at a fixed rate, drawing interpolated by alpha.
        const double alpha = timestep.update(frame_dt, [&](double step) {
            const double t0 = (*window)->time();
            physics.step(scene.registry, static_cast<float>(step));
            step_ms_sum += ((*window)->time() - t0) * 1000.0;
            ++step_count;
        });

        if (now >= next_title) {
            char title[128];
            std::snprintf(title, sizeof title, "engine - physics playground | %zu bodies | step %.3f ms",
                          scene.bodies, step_count > 0 ? step_ms_sum / step_count : 0.0);
            (*window)->set_title(title);
            step_ms_sum = 0.0;
            step_count = 0;
            next_title = now + 0.5;
        }

        if (now >= next_reload) {
            assets.reload_changed();
            next_reload = now + 1.0;
        }

        (*renderer)->render(scene.registry, camera, lens, static_cast<float>(alpha),
                            (*window)->framebuffer_size());
        (*window)->swap_buffers();
    }
    return EXIT_SUCCESS;
}
