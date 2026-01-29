#include "engine/physics/narrowphase.hpp"

#include <cmath>

namespace engine::physics {

namespace {
constexpr float kEpsilon = 1e-6f;
}

std::optional<Contact> collide_sphere_sphere(const glm::vec3& ca, float ra, const glm::vec3& cb,
                                             float rb) {
    const glm::vec3 d = cb - ca;
    const float sum = ra + rb;
    const float dist2 = glm::dot(d, d);
    if (dist2 >= sum * sum) return std::nullopt;

    const float dist = std::sqrt(dist2);
    const glm::vec3 n = dist > kEpsilon ? d / dist : glm::vec3{0.0f, 1.0f, 0.0f};
    Contact c;
    c.normal = n;
    c.depth = sum - dist;
    c.point = ca + n * (ra - c.depth * 0.5f);
    return c;
}

std::optional<Contact> collide_sphere_box(const glm::vec3& center, float radius,
                                          const glm::vec3& box_center, const glm::vec3& half) {
    const glm::vec3 bmin = box_center - half;
    const glm::vec3 bmax = box_center + half;
    const glm::vec3 q = glm::clamp(center, bmin, bmax);

    if (q != center) {
        const glm::vec3 d = q - center;
        const float dist2 = glm::dot(d, d);
        if (dist2 >= radius * radius) return std::nullopt;
        const float dist = std::sqrt(dist2);
        Contact c;
        c.normal = d / dist;
        c.depth = radius - dist;
        c.point = (q + center + c.normal * radius) * 0.5f;
        return c;
    }

    // Center inside the box or on its surface: leave through the nearest face.
    // Fixed order x-, x+, y-, y+, z-, z+; strict '<' makes the first one win ties.
    float best = center.x - bmin.x;
    glm::vec3 f{-1.0f, 0.0f, 0.0f};
    auto consider = [&](float dist, const glm::vec3& outward) {
        if (dist < best) {
            best = dist;
            f = outward;
        }
    };
    consider(bmax.x - center.x, {1.0f, 0.0f, 0.0f});
    consider(center.y - bmin.y, {0.0f, -1.0f, 0.0f});
    consider(bmax.y - center.y, {0.0f, 1.0f, 0.0f});
    consider(center.z - bmin.z, {0.0f, 0.0f, -1.0f});
    consider(bmax.z - center.z, {0.0f, 0.0f, 1.0f});

    Contact c;
    c.normal = -f;
    c.depth = radius + best;
    c.point = center + f * ((best - radius) * 0.5f);
    return c;
}

std::optional<Contact> collide_box_box(const glm::vec3& ca, const glm::vec3& ha,
                                       const glm::vec3& cb, const glm::vec3& hb) {
    const glm::vec3 delta = cb - ca;
    const glm::vec3 overlap = ha + hb - glm::abs(delta);
    if (overlap.x <= 0.0f || overlap.y <= 0.0f || overlap.z <= 0.0f) return std::nullopt;

    int axis = 0;
    if (overlap.y < overlap[axis]) axis = 1;
    if (overlap.z < overlap[axis]) axis = 2;

    Contact c;
    c.normal = glm::vec3{0.0f};
    c.normal[axis] = delta[axis] < 0.0f ? -1.0f : 1.0f;
    c.depth = overlap[axis];
    const glm::vec3 lo = glm::max(ca - ha, cb - hb);
    const glm::vec3 hi = glm::min(ca + ha, cb + hb);
    c.point = (lo + hi) * 0.5f;
    return c;
}

std::optional<Contact> collide(const Collider& a, const glm::vec3& pa, const Collider& b,
                               const glm::vec3& pb) {
    if (a.type == ShapeType::Sphere) {
        if (b.type == ShapeType::Sphere) return collide_sphere_sphere(pa, a.radius, pb, b.radius);
        return collide_sphere_box(pa, a.radius, pb, b.half_extents);
    }
    if (b.type == ShapeType::Box) return collide_box_box(pa, a.half_extents, pb, b.half_extents);
    auto c = collide_sphere_box(pb, b.radius, pa, a.half_extents);
    if (c) c->normal = -c->normal;
    return c;
}

}  // namespace engine::physics
