// Physics benchmark: PhysicsWorld::step and the broadphase with 1k and 5k bodies.
// Build in Release; see docs/benchmarks.md for the exact commands.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>

#include "engine/ecs/registry.hpp"
#include "engine/math/transform.hpp"
#include "engine/physics/broadphase.hpp"
#include "engine/physics/components.hpp"
#include "engine/physics/world.hpp"

namespace {

using engine::Entity;
using engine::Registry;
using engine::math::Transform;
using namespace engine::physics;

constexpr float kDt = 1.0f / 60.0f;
constexpr int kSteps = 300;      // 5 simulated seconds: fall, impact, settle
constexpr int kRuns = 3;         // fresh scene each run; samples are pooled
constexpr int kBroadphaseRuns = 21;
constexpr int kFootprint = 20;   // bodies per layer side
constexpr float kSpacing = 1.05f;
constexpr float kLayerHeight = 1.5f;

using Clock = std::chrono::steady_clock;

double ms_since(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

void add_static_box(Registry& r, const glm::vec3& center, const glm::vec3& half) {
    const Entity e = r.create();
    Transform t;
    t.position = center;
    r.add<Transform>(e, t);
    r.add<Collider>(e, Collider::box(half));
}

// Floor and four walls around a kFootprint x kFootprint grid, then `count` bodies (alternating
// spheres and boxes, radius / half extent 0.5) in layers above the floor.
void build_scene(Registry& r, int count) {
    const float side = static_cast<float>(kFootprint) * kSpacing;
    const float half = side * 0.5f + 1.0f;
    const float wall_h = 200.0f;
    add_static_box(r, {0.0f, -1.0f, 0.0f}, {half + 1.0f, 1.0f, half + 1.0f});
    add_static_box(r, {-half - 1.0f, wall_h, 0.0f}, {1.0f, wall_h, half + 1.0f});
    add_static_box(r, {half + 1.0f, wall_h, 0.0f}, {1.0f, wall_h, half + 1.0f});
    add_static_box(r, {0.0f, wall_h, -half - 1.0f}, {half + 1.0f, wall_h, 1.0f});
    add_static_box(r, {0.0f, wall_h, half + 1.0f}, {half + 1.0f, wall_h, 1.0f});

    for (int i = 0; i < count; ++i) {
        const int layer = i / (kFootprint * kFootprint);
        const int in_layer = i % (kFootprint * kFootprint);
        const int ix = in_layer % kFootprint;
        const int iz = in_layer / kFootprint;
        const Entity e = r.create();
        Transform t;
        t.position = {(static_cast<float>(ix) - kFootprint * 0.5f + 0.5f) * kSpacing,
                      1.0f + static_cast<float>(layer) * kLayerHeight,
                      (static_cast<float>(iz) - kFootprint * 0.5f + 0.5f) * kSpacing};
        r.add<Transform>(e, t);
        r.add<RigidBody>(e, RigidBody::with_mass(1.0f));
        r.add<Collider>(e, (i % 2 == 0) ? Collider::sphere(0.5f)
                                        : Collider::box({0.5f, 0.5f, 0.5f}));
    }
}

struct Stats {
    double min_ms;
    double median_ms;
    double max_ms;
};

Stats summarize(std::vector<double> s) {
    std::sort(s.begin(), s.end());
    return {s.front(), s[s.size() / 2], s.back()};
}

std::vector<BroadphaseProxy> make_proxies(Registry& r) {
    std::vector<BroadphaseProxy> proxies;
    r.view<const Transform, const Collider>().each(
        [&](Entity e, const Transform& t, const Collider& c) {
            proxies.push_back({e, collider_aabb(c, t.position), !r.has<RigidBody>(e)});
        });
    return proxies;
}

void bench(int bodies) {
    std::vector<double> step_ms;
    std::size_t final_contacts = 0;
    std::size_t entities = 0;
    for (int run = 0; run < kRuns; ++run) {
        Registry registry;
        build_scene(registry, bodies);
        entities = static_cast<std::size_t>(bodies) + 5;
        PhysicsWorld world;
        for (int i = 0; i < kSteps; ++i) {
            const auto start = Clock::now();
            world.step(registry, kDt);
            step_ms.push_back(ms_since(start));
        }
        final_contacts = world.contacts().size();
    }

    // Broadphase on the settled scene: sweep-and-prune vs brute force, same input.
    Registry registry;
    build_scene(registry, bodies);
    PhysicsWorld world;
    for (int i = 0; i < kSteps; ++i) {
        world.step(registry, kDt);
    }
    const std::vector<BroadphaseProxy> proxies = make_proxies(registry);
    BroadphaseOptions opt;
    opt.skip_static_pairs = true;
    std::vector<double> sap_ms;
    std::vector<double> brute_ms;
    std::size_t pairs = 0;
    for (int i = 0; i < kBroadphaseRuns; ++i) {
        auto start = Clock::now();
        pairs = sweep_and_prune(proxies, opt).size();
        sap_ms.push_back(ms_since(start));
        start = Clock::now();
        const std::size_t brute_pairs = brute_force_pairs(proxies, opt).size();
        brute_ms.push_back(ms_since(start));
        if (brute_pairs != pairs) {
            std::printf("ERROR: sweep_and_prune (%zu) and brute force (%zu) disagree\n", pairs,
                        brute_pairs);
        }
    }

    const Stats st = summarize(step_ms);
    const Stats sp = summarize(sap_ms);
    const Stats bf = summarize(brute_ms);
    std::printf("| %d | PhysicsWorld::step | %zu | %.3f | %.3f | %.3f |\n", bodies, step_ms.size(),
                st.min_ms, st.median_ms, st.max_ms);
    std::printf("| %d | sweep_and_prune (settled) | %d | %.3f | %.3f | %.3f |\n", bodies,
                kBroadphaseRuns, sp.min_ms, sp.median_ms, sp.max_ms);
    std::printf("| %d | brute_force_pairs (settled) | %d | %.3f | %.3f | %.3f |\n", bodies,
                kBroadphaseRuns, bf.min_ms, bf.median_ms, bf.max_ms);
    std::fprintf(stderr, "bodies %d: entities %zu, contacts after last step %zu, broadphase pairs %zu\n",
                 bodies, entities, final_contacts, pairs);
}

void print_build_info() {
#if defined(__clang__)
    std::printf("compiler: clang %s\n", __clang_version__);
#elif defined(__GNUC__)
    std::printf("compiler: gcc %s\n", __VERSION__);
#elif defined(_MSC_VER)
    std::printf("compiler: msvc %d\n", _MSC_FULL_VER);
#endif
#ifdef NDEBUG
    std::printf("assertions: off (NDEBUG)\n");
#else
    std::printf("assertions: ON - not a Release build, numbers are not representative\n");
#endif
}

}  // namespace

int main() {
    print_build_info();
    std::printf("steps per run: %d (dt = 1/60), runs: %d\n\n", kSteps, kRuns);
    std::printf("| Bodies | Scenario | Samples | Min (ms) | Median (ms) | Max (ms) |\n");
    std::printf("|---|---|---|---|---|---|\n");
    bench(1000);
    bench(5000);
    return 0;
}
