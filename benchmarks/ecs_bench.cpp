// ECS micro-benchmark: 100k entities with Position + Velocity.
// Build in Release; see docs/benchmarks.md for the exact commands.
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <utility>
#include <vector>

#include "engine/ecs/registry.hpp"

namespace {

struct Position {
    float x, y, z;
};
struct Velocity {
    float x, y, z;
};

constexpr std::size_t kEntities = 100000;
constexpr int kBuildRuns = 15;
constexpr int kWarmup = 5;
constexpr int kUpdatePasses = 101;
constexpr float kDt = 1.0f / 60.0f;

using Clock = std::chrono::steady_clock;

double ms_since(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

struct Stats {
    double min_ms;
    double median_ms;
};

Stats summarize(std::vector<double> samples) {
    std::sort(samples.begin(), samples.end());
    return {samples.front(), samples[samples.size() / 2]};
}

void populate(engine::Registry& registry, std::vector<engine::Entity>& entities) {
    entities.clear();
    entities.reserve(kEntities);
    for (std::size_t i = 0; i < kEntities; ++i) {
        const engine::Entity e = registry.create();
        registry.add<Position>(e, static_cast<float>(i), 0.0f, 0.0f);
        registry.add<Velocity>(e, 1.0f, 2.0f, 3.0f);
        entities.push_back(e);
    }
}

void update(engine::Registry& registry) {
    registry.view<Position, const Velocity>().each([](Position& p, const Velocity& v) {
        p.x += v.x * kDt;
        p.y += v.y * kDt;
        p.z += v.z * kDt;
    });
}

double checksum(engine::Registry& registry) {
    double sum = 0.0;
    registry.view<Position>().each([&sum](const Position& p) { sum += p.x + p.y + p.z; });
    return sum;
}

void print_row(const char* name, int runs, Stats s) {
    const double ns_per_entity = s.median_ms * 1e6 / static_cast<double>(kEntities);
    std::printf("| %s | %d | %.3f | %.3f | %.1f |\n", name, runs, s.min_ms, s.median_ms,
                ns_per_entity);
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
    std::printf("entities: %zu, update passes: %d (+%d warm-up), build/destroy runs: %d\n\n",
                kEntities, kUpdatePasses, kWarmup, kBuildRuns);

    std::vector<double> create_ms;
    std::vector<double> destroy_ms;
    std::vector<engine::Entity> entities;
    for (int run = 0; run < kBuildRuns; ++run) {
        engine::Registry registry;
        auto start = Clock::now();
        populate(registry, entities);
        create_ms.push_back(ms_since(start));

        start = Clock::now();
        for (const engine::Entity e : entities) {
            registry.destroy(e);
        }
        destroy_ms.push_back(ms_since(start));
    }

    engine::Registry registry;
    populate(registry, entities);
    for (int i = 0; i < kWarmup; ++i) {
        update(registry);
    }
    std::vector<double> update_ms;
    for (int i = 0; i < kUpdatePasses; ++i) {
        const auto start = Clock::now();
        update(registry);
        update_ms.push_back(ms_since(start));
    }

    std::printf("| Scenario (100k entities) | Runs | Min (ms) | Median (ms) | Median ns/entity |\n");
    std::printf("|---|---|---|---|---|\n");
    print_row("create + add Position + add Velocity", kBuildRuns, summarize(create_ms));
    print_row("view<Position, const Velocity>().each", kUpdatePasses, summarize(update_ms));
    print_row("destroy (removes 2 components each)", kBuildRuns, summarize(destroy_ms));
    // Printed so the compiler cannot discard the updates.
    std::printf("\nchecksum: %.6g\n", checksum(registry));
    return 0;
}
