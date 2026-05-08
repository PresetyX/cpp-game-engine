#pragma once

#include <optional>

#include "engine/math/glm.hpp"

namespace engine::render {

// All angles are in radians. Pure math: no window or GL needed.
struct PerspectiveCamera {
    float fov_y = glm::radians(60.0f);
    float near_plane = 0.1f;
    float far_plane = 200.0f;

    // OpenGL clip space (z in [-1, 1]). Empty when the framebuffer has zero area (minimized
    // window): the caller must skip the frame instead of dividing by zero.
    std::optional<glm::mat4> projection(int width, int height) const;
};

// Camera orbiting around `target`. yaw 0 looks from +Z towards the target; positive pitch moves
// the eye above the target. Pitch is clamped to +-max_pitch so the view never flips at the poles.
struct OrbitCamera {
    static constexpr float max_pitch = glm::radians(89.0f);
    static constexpr float min_distance = 0.1f;
    static constexpr float max_distance = 1000.0f;

    glm::vec3 target{0.0f};
    float distance = 10.0f;
    float yaw = 0.0f;
    float pitch = glm::radians(20.0f);

    // Adds to yaw/pitch (e.g. mouse delta times a sensitivity) and clamps pitch.
    void orbit(float delta_yaw, float delta_pitch);
    // Multiplies the distance (factor < 1 zooms in) and clamps it.
    void zoom(float factor);
    // Moves the target (panning); the eye follows.
    void pan(const glm::vec3& delta) { target += delta; }

    glm::vec3 eye() const;
    glm::mat4 view() const;
};

}  // namespace engine::render
