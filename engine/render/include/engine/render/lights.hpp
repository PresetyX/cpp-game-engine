#pragma once

#include <array>
#include <cstddef>

#include "engine/ecs/registry.hpp"
#include "engine/math/glm.hpp"

namespace engine::render {

// Must match MAX_POINT_LIGHTS in the Phong fragment shader (checked by a test).
inline constexpr std::size_t kMaxPointLights = 8;

struct PointLightData {
    glm::vec3 position{0.0f};
    float range = 1.0f;
    glm::vec3 color{0.0f};  // color * intensity
};

// Lights in the form the shader consumes. All colors are already multiplied by intensity.
struct LightBlock {
    glm::vec3 direction{0.0f, -1.0f, 0.0f};  // normalized, direction the light travels
    glm::vec3 directional_color{0.0f};       // black when the scene has no directional light
    glm::vec3 ambient{0.12f};
    std::size_t point_count = 0;
    std::array<PointLightData, kMaxPointLights> points{};
};

// Builds the block for one frame. The directional light is the one on the lowest entity index
// (a zero direction falls back to straight down). Point lights need a Transform (interpolated
// with `alpha` like meshes); if there are more than kMaxPointLights, the ones closest to
// `camera_position` win, ties broken by entity index, and the result is ordered the same way.
// Point lights with range <= 0 or zero color are skipped.
LightBlock pack_lights(const Registry& registry, const glm::vec3& camera_position, float alpha);

}  // namespace engine::render
