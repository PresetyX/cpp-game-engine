#pragma once

#include <vector>

#include "engine/ecs/entity.hpp"
#include "engine/math/aabb.hpp"
#include "engine/physics/components.hpp"

namespace engine::physics {

struct BroadphaseProxy {
    Entity entity;
    math::Aabb bounds;
    bool is_static = false;
};

// Canonical orientation: (index, generation) of `a` is lower than that of `b`.
struct BroadphasePair {
    Entity a;
    Entity b;
};

struct BroadphaseOptions {
    bool skip_static_pairs = false;  // drops pairs where both proxies are static
};

// World-space AABB of a collider centered on `position`.
math::Aabb collider_aabb(const Collider& c, const glm::vec3& position);

// Sweep-and-prune on the x axis. Each unordered pair is emitted exactly once, oriented
// (lower key, higher key) and sorted by (a.index, a.generation, b.index, b.generation), so the
// result is identical for any permutation of the input. Touching boxes count as overlapping.
// Preconditions: entities are unique; bounds contain no NaN (Debug asserts; in Release proxies
// with NaN are ignored, which matches brute force). +-inf is allowed.
std::vector<BroadphasePair> sweep_and_prune(const std::vector<BroadphaseProxy>& proxies,
                                            const BroadphaseOptions& options = {});

// O(n^2) reference with the same output contract.
std::vector<BroadphasePair> brute_force_pairs(const std::vector<BroadphaseProxy>& proxies,
                                              const BroadphaseOptions& options = {});

}  // namespace engine::physics
