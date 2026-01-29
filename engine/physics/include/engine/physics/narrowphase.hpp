#pragma once

#include <optional>

#include "engine/math/glm.hpp"
#include "engine/physics/components.hpp"

namespace engine::physics {

// Contact between shapes A and B. `normal` is unit length and points from A to B (the solver
// pushes B along +normal and A along -normal). `depth` is always > 0. `point` is the midpoint
// between the deepest point of A inside B and the surface point of B along -normal.
struct Contact {
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float depth = 0.0f;
    glm::vec3 point{0.0f};
};

// Pure functions: no Registry. Exact touching (zero penetration) is NOT a contact.
std::optional<Contact> collide_sphere_sphere(const glm::vec3& ca, float ra, const glm::vec3& cb,
                                             float rb);
// A is the sphere, B is the box (axis-aligned).
std::optional<Contact> collide_sphere_box(const glm::vec3& center, float radius,
                                          const glm::vec3& box_center, const glm::vec3& half);
std::optional<Contact> collide_box_box(const glm::vec3& ca, const glm::vec3& ha,
                                       const glm::vec3& cb, const glm::vec3& hb);

// Dispatches on the shape types. Box-Sphere is Sphere-Box with the normal flipped.
std::optional<Contact> collide(const Collider& a, const glm::vec3& pa, const Collider& b,
                               const glm::vec3& pb);

}  // namespace engine::physics
