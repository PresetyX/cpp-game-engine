#pragma once

#include "engine/ecs/registry.hpp"
#include "engine/math/glm.hpp"

namespace engine::physics {

// Copies Transform into PreviousTransform for every entity that has both. Call once at the
// start of each fixed step, before any movement. Entities without PreviousTransform are skipped.
void store_previous_transforms(Registry& registry);

// Semi-implicit (symplectic) Euler for every entity with Transform + RigidBody:
//   v += (gravity * gravity_scale + force * inverse_mass) * dt     (only if inverse_mass > 0)
//   v *= 1 / (1 + linear_damping * dt)                             (stable for any dt)
//   position += v * dt                                             (uses the updated v)
// Clears RigidBody::force afterwards. Rotation is not integrated.
void integrate_bodies(Registry& registry, float dt, const glm::vec3& gravity);

}  // namespace engine::physics
