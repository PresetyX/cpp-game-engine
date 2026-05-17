#pragma once

#include <vector>

#include "engine/ecs/registry.hpp"
#include "engine/math/transform.hpp"
#include "engine/render/components.hpp"

namespace engine::render {

// Everything the renderer needs to draw one entity, already resolved on the CPU.
struct DrawItem {
    Entity entity;
    glm::mat4 model{1.0f};
    glm::mat3 normal_matrix{1.0f};  // transpose(inverse(mat3(model))); identity if singular
    assets::Handle<assets::Mesh> mesh;
    assets::Handle<assets::Image> texture;
    Material material;
};

// The transform to render with: interpolate(previous, current, alpha) when the entity has a
// PreviousTransform, the current Transform otherwise. alpha is clamped to [0, 1]. Returns false
// if the entity has no Transform.
bool render_transform(const Registry& registry, Entity e, float alpha, math::Transform& out);

// Collects every entity with MeshRenderer + Transform and a valid mesh handle. Sorted by
// (texture, mesh, entity) so draws sharing GPU state are adjacent and the order never depends on
// hash-map iteration or on the order components were added. `out` is cleared first (reuse it
// across frames to avoid allocations).
void collect_draw_items(const Registry& registry, float alpha, std::vector<DrawItem>& out);

inline std::vector<DrawItem> collect_draw_items(const Registry& registry, float alpha) {
    std::vector<DrawItem> out;
    collect_draw_items(registry, alpha, out);
    return out;
}

}  // namespace engine::render
