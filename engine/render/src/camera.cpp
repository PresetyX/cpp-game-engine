#include "engine/render/camera.hpp"

#include <algorithm>
#include <cmath>

namespace engine::render {

std::optional<glm::mat4> PerspectiveCamera::projection(int width, int height) const {
    if (width <= 0 || height <= 0) {
        return std::nullopt;
    }
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    return glm::perspective(fov_y, aspect, near_plane, far_plane);
}

void OrbitCamera::orbit(float delta_yaw, float delta_pitch) {
    yaw = std::remainder(yaw + delta_yaw, glm::two_pi<float>());
    pitch = std::clamp(pitch + delta_pitch, -max_pitch, max_pitch);
}

void OrbitCamera::zoom(float factor) {
    distance = std::clamp(distance * factor, min_distance, max_distance);
}

glm::vec3 OrbitCamera::eye() const {
    const float cp = std::cos(pitch);
    return target + distance * glm::vec3(cp * std::sin(yaw), std::sin(pitch), cp * std::cos(yaw));
}

glm::mat4 OrbitCamera::view() const {
    return glm::lookAt(eye(), target, glm::vec3(0.0f, 1.0f, 0.0f));
}

}  // namespace engine::render
