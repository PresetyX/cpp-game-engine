#include "engine/physics/solver.hpp"

#include <algorithm>
#include <cmath>

namespace engine::physics {

namespace {

glm::vec3 velocity_of(const SolverBody& b) {
    return b.velocity != nullptr ? *b.velocity : glm::vec3(0.0f);
}

// Applies impulse j to b with sign +1 (b) or -1 (a). No-op for static/kinematic bodies.
void apply_impulse(const SolverBody& b, const glm::vec3& j, float sign) {
    if (b.inverse_mass > 0.0f && b.velocity != nullptr) {
        *b.velocity += j * (sign * b.inverse_mass);
    }
}

struct Accumulator {
    float normal_impulse = 0.0f;
    glm::vec3 tangent_impulse{0.0f};
    float velocity_bias = 0.0f;
    float inv_k = 0.0f;  // 1 / (invA + invB)
};

}  // namespace

float mix_friction(float a, float b) {
    return std::sqrt(a * b);
}

float mix_restitution(float a, float b) {
    return std::max(a, b);
}

void solve_contacts(std::vector<ContactConstraint>& constraints, const SolverSettings& settings) {
    std::vector<Accumulator> acc(constraints.size());

    // Pre-solve: effective mass and restitution target from the closing speed before any impulse.
    for (std::size_t i = 0; i < constraints.size(); ++i) {
        const ContactConstraint& c = constraints[i];
        const float k = c.a.inverse_mass + c.b.inverse_mass;
        acc[i].inv_k = k > 0.0f ? 1.0f / k : 0.0f;
        const float vn0 = glm::dot(velocity_of(c.b) - velocity_of(c.a), c.normal);
        if (vn0 < -settings.restitution_threshold) {
            acc[i].velocity_bias = -c.restitution * vn0;
        }
    }

    for (int it = 0; it < settings.velocity_iterations; ++it) {
        for (std::size_t i = 0; i < constraints.size(); ++i) {
            const ContactConstraint& c = constraints[i];
            Accumulator& s = acc[i];

            // Normal: drive the relative normal speed to the bias; accumulated impulse >= 0.
            {
                const float vn = glm::dot(velocity_of(c.b) - velocity_of(c.a), c.normal);
                const float updated =
                    std::max(s.normal_impulse + (s.velocity_bias - vn) * s.inv_k, 0.0f);
                const float delta = updated - s.normal_impulse;
                s.normal_impulse = updated;
                const glm::vec3 j = c.normal * delta;
                apply_impulse(c.a, j, -1.0f);
                apply_impulse(c.b, j, +1.0f);
            }

            // Coulomb friction: remove the tangential relative velocity, clamped to mu * Pn.
            {
                const glm::vec3 rv = velocity_of(c.b) - velocity_of(c.a);
                const glm::vec3 vt = rv - c.normal * glm::dot(rv, c.normal);
                glm::vec3 updated = s.tangent_impulse - vt * s.inv_k;
                const float limit = c.friction * s.normal_impulse;
                const float len = glm::length(updated);
                if (len > limit) {
                    updated = len > 0.0f ? updated * (limit / len) : glm::vec3(0.0f);
                }
                const glm::vec3 j = updated - s.tangent_impulse;
                s.tangent_impulse = updated;
                apply_impulse(c.a, j, -1.0f);
                apply_impulse(c.b, j, +1.0f);
            }
        }
    }

    // Position correction by direct projection, split by inverse mass.
    for (std::size_t i = 0; i < constraints.size(); ++i) {
        const ContactConstraint& c = constraints[i];
        const float excess = std::max(c.depth - settings.penetration_slop, 0.0f);
        const glm::vec3 shift = c.normal * (excess * settings.correction_percent * acc[i].inv_k);
        if (c.a.inverse_mass > 0.0f && c.a.position != nullptr) {
            *c.a.position -= shift * c.a.inverse_mass;
        }
        if (c.b.inverse_mass > 0.0f && c.b.position != nullptr) {
            *c.b.position += shift * c.b.inverse_mass;
        }
    }
}

}  // namespace engine::physics
