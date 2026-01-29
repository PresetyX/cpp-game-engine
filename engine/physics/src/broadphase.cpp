#include "engine/physics/broadphase.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <tuple>

namespace engine::physics {

namespace {

auto key(Entity e) {
    return std::make_tuple(e.index, e.generation);
}

bool has_nan(const math::Aabb& b) {
    return std::isnan(b.min.x) || std::isnan(b.min.y) || std::isnan(b.min.z) ||
           std::isnan(b.max.x) || std::isnan(b.max.y) || std::isnan(b.max.z);
}

bool pair_less(const BroadphasePair& l, const BroadphasePair& r) {
    return std::make_tuple(key(l.a), key(l.b)) < std::make_tuple(key(r.a), key(r.b));
}

BroadphasePair make_pair(const BroadphaseProxy& p, const BroadphaseProxy& q) {
    return key(p.entity) < key(q.entity) ? BroadphasePair{p.entity, q.entity}
                                         : BroadphasePair{q.entity, p.entity};
}

bool skipped(const BroadphaseProxy& p, const BroadphaseProxy& q, const BroadphaseOptions& o) {
    return o.skip_static_pairs && p.is_static && q.is_static;
}

}  // namespace

math::Aabb collider_aabb(const Collider& c, const glm::vec3& position) {
    const glm::vec3 half = c.type == ShapeType::Sphere ? glm::vec3(c.radius) : c.half_extents;
    return math::Aabb::from_center_half_extents(position, half);
}

std::vector<BroadphasePair> sweep_and_prune(const std::vector<BroadphaseProxy>& proxies,
                                            const BroadphaseOptions& options) {
    std::vector<const BroadphaseProxy*> sorted;
    sorted.reserve(proxies.size());
    for (const BroadphaseProxy& p : proxies) {
        assert(!has_nan(p.bounds));
        if (!has_nan(p.bounds)) {
            sorted.push_back(&p);
        }
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const BroadphaseProxy* l, const BroadphaseProxy* r) {
                  return std::make_tuple(l->bounds.min.x, key(l->entity)) <
                         std::make_tuple(r->bounds.min.x, key(r->entity));
              });
#ifndef NDEBUG
    {
        std::vector<Entity> ids;
        for (const BroadphaseProxy* p : sorted) {
            ids.push_back(p->entity);
        }
        std::sort(ids.begin(), ids.end(), [](Entity l, Entity r) { return key(l) < key(r); });
        assert(std::adjacent_find(ids.begin(), ids.end()) == ids.end() && "duplicate entity");
    }
#endif

    std::vector<BroadphasePair> out;
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        const BroadphaseProxy& a = *sorted[i];
        for (std::size_t j = i + 1; j < sorted.size(); ++j) {
            const BroadphaseProxy& b = *sorted[j];
            if (b.bounds.min.x > a.bounds.max.x) {
                break;
            }
            if (math::overlaps(a.bounds, b.bounds) && !skipped(a, b, options)) {
                out.push_back(make_pair(a, b));
            }
        }
    }
    std::sort(out.begin(), out.end(), pair_less);
    return out;
}

std::vector<BroadphasePair> brute_force_pairs(const std::vector<BroadphaseProxy>& proxies,
                                              const BroadphaseOptions& options) {
    std::vector<BroadphasePair> out;
    for (std::size_t i = 0; i < proxies.size(); ++i) {
        for (std::size_t j = i + 1; j < proxies.size(); ++j) {
            if (math::overlaps(proxies[i].bounds, proxies[j].bounds) &&
                !skipped(proxies[i], proxies[j], options)) {
                out.push_back(make_pair(proxies[i], proxies[j]));
            }
        }
    }
    std::sort(out.begin(), out.end(), pair_less);
    return out;
}

}  // namespace engine::physics
