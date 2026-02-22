#pragma once

#include <vector>

#include "engine/ecs/entity.hpp"
#include "engine/ecs/registry.hpp"
#include "engine/math/glm.hpp"
#include "engine/physics/narrowphase.hpp"

namespace engine::physics {

struct PhysicsSettings {
    glm::vec3 gravity{0.0f, -9.81f, 0.0f};
    int velocity_iterations = 10;
    float penetration_slop = 0.01f;
    float correction_percent = 0.4f;
    float restitution_threshold = 1.0f;  // m/s
};

// Overlap found during the last step. Canonical orientation: a < b (index, generation) and
// contact.normal points from a to b. `trigger` is true if either collider is a trigger; such
// pairs get no physical response.
struct ContactPair {
    Entity a;
    Entity b;
    Contact contact;
    bool trigger = false;
};

// Owns no entities: all state lives in the Registry. Call step() once per FixedTimestep tick.
class PhysicsWorld {
public:
    explicit PhysicsWorld(const PhysicsSettings& settings = {}) : settings_(settings) {}

    // 1. store previous transforms   2. integrate (gravity, forces, damping)
    // 3. broadphase + narrowphase    4. solve contacts (sequential impulses) + position correction
    // Static = Collider without RigidBody; kinematic = RigidBody with inverse_mass == 0.
    // Pairs with no dynamic body (static-static, kinematic-static, kinematic-kinematic) are not
    // resolved and, unless a trigger is involved, are not reported either.
    // Same inputs give bit-identical results on the same build and platform.
    void step(Registry& registry, float dt);

    // Overlaps of the last step (solid and trigger), in canonical pair order. Cleared by step().
    const std::vector<ContactPair>& contacts() const { return contacts_; }

    PhysicsSettings& settings() { return settings_; }
    const PhysicsSettings& settings() const { return settings_; }

private:
    PhysicsSettings settings_;
    std::vector<ContactPair> contacts_;
};

}  // namespace engine::physics
