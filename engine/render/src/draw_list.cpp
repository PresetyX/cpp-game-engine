#include "engine/render/draw_list.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <tuple>

namespace engine::render {

namespace {

float clamp01(float a) {
    return std::isnan(a) ? 0.0f : std::clamp(a, 0.0f, 1.0f);
}

std::tuple<std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t>
sort_key(const DrawItem& d) {
    return {d.texture.index, d.texture.generation, d.mesh.index,
            d.mesh.generation, d.entity.index,     d.entity.generation};
}

glm::mat3 normal_matrix_of(const glm::mat4& model) {
    const glm::mat3 m(model);
    const float det = glm::determinant(m);
    if (!(std::abs(det) > 1e-12f)) {
        // NOLINTNEXTLINE(modernize-return-braced-init-list): glm::mat3(float) is explicit
        return glm::mat3(1.0f);
    }
    return glm::transpose(glm::inverse(m));
}

}  // namespace

bool render_transform(const Registry& registry, Entity e, float alpha, math::Transform& out) {
    const auto* curr = registry.try_get<math::Transform>(e);
    if (curr == nullptr) {
        return false;
    }
    const auto* prev = registry.try_get<math::PreviousTransform>(e);
    out = prev != nullptr ? math::interpolate(prev->value, *curr, clamp01(alpha)) : *curr;
    return true;
}

void collect_draw_items(const Registry& registry, float alpha, std::vector<DrawItem>& out) {
    out.clear();
    registry.view<const MeshRenderer, const math::Transform>().each(
        [&](Entity e, const MeshRenderer& mr, const math::Transform&) {
            if (!mr.mesh.valid()) {
                return;
            }
            math::Transform t;
            if (!render_transform(registry, e, alpha, t)) {
                return;
            }
            DrawItem item;
            item.entity = e;
            item.model = t.to_matrix();
            item.normal_matrix = normal_matrix_of(item.model);
            item.mesh = mr.mesh;
            item.texture = mr.texture;
            item.material = mr.material;
            out.push_back(item);
        });
    std::sort(out.begin(), out.end(),
              [](const DrawItem& a, const DrawItem& b) { return sort_key(a) < sort_key(b); });
}

}  // namespace engine::render
