#include "engine/render/lights.hpp"

#include <algorithm>
#include <vector>

#include "engine/render/components.hpp"
#include "engine/render/draw_list.hpp"

namespace engine::render {

namespace {

struct Candidate {
    float distance_sq;
    Entity entity;
    PointLightData data;
};

bool before(Entity a, Entity b) {
    return a.index != b.index ? a.index < b.index : a.generation < b.generation;
}

}  // namespace

LightBlock pack_lights(const Registry& registry, const glm::vec3& camera_position, float alpha) {
    LightBlock block;

    Entity best = null_entity;
    const DirectionalLight* dir = nullptr;
    registry.view<const DirectionalLight>().each([&](Entity e, const DirectionalLight& l) {
        if (dir == nullptr || before(e, best)) {
            dir = &l;
            best = e;
        }
    });
    if (dir != nullptr) {
        const float len = glm::length(dir->direction);
        block.direction = len > 1e-6f ? dir->direction / len : glm::vec3(0.0f, -1.0f, 0.0f);
        block.directional_color = dir->color * dir->intensity;
        block.ambient = dir->ambient;
    }

    std::vector<Candidate> candidates;
    registry.view<const PointLight>().each([&](Entity e, const PointLight& l) {
        const glm::vec3 color = l.color * l.intensity;
        if (!(l.range > 0.0f) || (color.x <= 0.0f && color.y <= 0.0f && color.z <= 0.0f)) {
            return;
        }
        math::Transform t;
        if (!render_transform(registry, e, alpha, t)) {
            return;
        }
        const glm::vec3 d = t.position - camera_position;
        candidates.push_back({glm::dot(d, d), e, {t.position, l.range, color}});
    });
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        if (a.distance_sq != b.distance_sq) {
            return a.distance_sq < b.distance_sq;
        }
        return before(a.entity, b.entity);
    });

    block.point_count = std::min(candidates.size(), kMaxPointLights);
    for (std::size_t i = 0; i < block.point_count; ++i) {
        block.points[i] = candidates[i].data;
    }
    return block;
}

}  // namespace engine::render
