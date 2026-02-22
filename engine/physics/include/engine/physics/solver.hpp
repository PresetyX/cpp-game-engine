#pragma once

#include <vector>

#include "engine/math/glm.hpp"

namespace engine::physics {

struct SolverSettings {
    int velocity_iterations = 10;
    float penetration_slop = 0.01f;     // m of penetration tolerated before position correction
    float correction_percent = 0.4f;    // fraction of the excess depth corrected per step, (0, 1]
    float restitution_threshold = 1.0f; // m/s; slower closing speeds do not bounce
};

// One side of a constraint. Static bodies have null pointers and inverse_mass 0; kinematic
// bodies have pointers but inverse_mass 0. Only bodies with inverse_mass > 0 are ever written.
struct SolverBody {
    glm::vec3* position = nullptr;
    glm::vec3* velocity = nullptr;
    float inverse_mass = 0.0f;
};

// Contact between a and b. `normal` is unit length and points from a to b.
// Precondition: a.inverse_mass + b.inverse_mass > 0.
struct ContactConstraint {
    SolverBody a;
    SolverBody b;
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float depth = 0.0f;
    float friction = 0.5f;     // already mixed
    float restitution = 0.0f;  // already mixed
};

// Mixing rules for two colliders: friction is the geometric mean, restitution the maximum.
float mix_friction(float a, float b);
float mix_restitution(float a, float b);

// Sequential impulses, fixed iteration count, no warm starting. Constraints are solved in the
// given order on every pass (normal first, then Coulomb friction), so the result is a pure
// function of the input order. Afterwards positions are corrected once by direct projection
// (does not change velocities).
void solve_contacts(std::vector<ContactConstraint>& constraints, const SolverSettings& settings);

}  // namespace engine::physics
