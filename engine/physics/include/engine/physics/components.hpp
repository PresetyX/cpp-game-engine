#pragma once

#include <cassert>

#include "engine/math/glm.hpp"

namespace engine::physics {

enum class ShapeType { Sphere, Box };

// Collision shape in the entity local space (centered on Transform::position).
// Box is axis-aligned in world space: rotation is ignored by the physics (translation only).
// Transform::scale is also ignored; the shape dimensions are the source of truth.
struct Collider {
    ShapeType type = ShapeType::Sphere;
    float radius = 0.5f;                       // Sphere
    glm::vec3 half_extents{0.5f, 0.5f, 0.5f};  // Box
    float friction = 0.5f;                     // Coulomb coefficient, >= 0
    float restitution = 0.0f;                  // [0, 1]
    // Trigger: overlaps are reported by PhysicsWorld::contacts() but never resolved.
    bool is_trigger = false;

    static Collider sphere(float radius, float friction = 0.5f, float restitution = 0.0f) {
        Collider c;
        c.type = ShapeType::Sphere;
        c.radius = radius;
        c.friction = friction;
        c.restitution = restitution;
        return c;
    }

    static Collider box(const glm::vec3& half_extents, float friction = 0.5f,
                        float restitution = 0.0f) {
        Collider c;
        c.type = ShapeType::Box;
        c.half_extents = half_extents;
        c.friction = friction;
        c.restitution = restitution;
        return c;
    }
};

// Dynamic body. An entity with a Collider and no RigidBody is static.
// inverse_mass == 0 makes the body kinematic: gravity and forces are ignored, but it still
// moves with its velocity and is treated as infinitely heavy by the solver.
struct RigidBody {
    glm::vec3 velocity{0.0f};
    glm::vec3 force{0.0f};  // accumulated external force (N); cleared after each integration
    float inverse_mass = 1.0f;
    float linear_damping = 0.0f;  // 1/s, >= 0
    float gravity_scale = 1.0f;

    static RigidBody with_mass(float mass) {
        assert(mass > 0.0f);
        RigidBody b;
        b.inverse_mass = 1.0f / mass;
        return b;
    }

    static RigidBody kinematic() {
        RigidBody b;
        b.inverse_mass = 0.0f;
        return b;
    }

    void add_force(const glm::vec3& f) { force += f; }
    void add_impulse(const glm::vec3& j) { velocity += j * inverse_mass; }
};

}  // namespace engine::physics
