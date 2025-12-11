#pragma once

#include "engine/math/glm.hpp"

namespace engine::math {

// Position, rotation and scale of an entity. to_matrix() composes T * R * S.
struct Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 to_matrix() const {
        const glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
        const glm::mat4 r = glm::mat4_cast(rotation);
        const glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
        return t * r * s;
    }
};

// Transform of the previous fixed simulation step, used to interpolate rendering.
struct PreviousTransform {
    Transform value{};
};

// alpha in [0, 1]: 0 gives prev, 1 gives curr.
inline Transform interpolate(const Transform& prev, const Transform& curr, float alpha) {
    Transform out;
    out.position = glm::mix(prev.position, curr.position, alpha);
    out.rotation = glm::slerp(prev.rotation, curr.rotation, alpha);
    out.scale = glm::mix(prev.scale, curr.scale, alpha);
    return out;
}

}  // namespace engine::math
