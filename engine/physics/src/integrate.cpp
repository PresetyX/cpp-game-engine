#include "engine/physics/integrate.hpp"

#include "engine/math/transform.hpp"
#include "engine/physics/components.hpp"

namespace engine::physics {

void store_previous_transforms(Registry& registry) {
    registry.view<math::PreviousTransform, const math::Transform>().each(
        [](math::PreviousTransform& prev, const math::Transform& curr) { prev.value = curr; });
}

void integrate_bodies(Registry& registry, float dt, const glm::vec3& gravity) {
    registry.view<math::Transform, RigidBody>().each([&](math::Transform& t, RigidBody& body) {
        if (body.inverse_mass > 0.0f) {
            body.velocity += (gravity * body.gravity_scale + body.force * body.inverse_mass) * dt;
        }
        body.velocity *= 1.0f / (1.0f + body.linear_damping * dt);
        t.position += body.velocity * dt;
        body.force = glm::vec3(0.0f);
    });
}

}  // namespace engine::physics
