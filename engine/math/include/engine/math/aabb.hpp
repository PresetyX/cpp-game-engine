#pragma once

#include "engine/math/glm.hpp"

namespace engine::math {

// Axis-aligned bounding box.
struct Aabb {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};

    static Aabb from_center_half_extents(const glm::vec3& center, const glm::vec3& half) {
        return Aabb{center - half, center + half};
    }

    glm::vec3 center() const { return (min + max) * 0.5f; }
    glm::vec3 half_extents() const { return (max - min) * 0.5f; }
};

// Boxes that merely touch on a face count as overlapping (this is meant for the
// broadphase; the narrowphase discards zero penetration).
inline bool overlaps(const Aabb& a, const Aabb& b) {
    return a.min.x <= b.max.x && a.max.x >= b.min.x &&
           a.min.y <= b.max.y && a.max.y >= b.min.y &&
           a.min.z <= b.max.z && a.max.z >= b.min.z;
}

inline bool contains(const Aabb& outer, const Aabb& inner) {
    return outer.min.x <= inner.min.x && outer.max.x >= inner.max.x &&
           outer.min.y <= inner.min.y && outer.max.y >= inner.max.y &&
           outer.min.z <= inner.min.z && outer.max.z >= inner.max.z;
}

}  // namespace engine::math
