#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "engine/ecs/registry.hpp"
#include "engine/math/transform.hpp"
#include "engine/render/draw_list.hpp"
#include "engine/render/lights.hpp"

using engine::Entity;
using engine::Registry;
using engine::assets::Handle;
using engine::assets::Image;
using engine::assets::Mesh;
using engine::math::PreviousTransform;
using engine::math::Transform;
using engine::render::collect_draw_items;
using engine::render::DirectionalLight;
using engine::render::DrawItem;
using engine::render::kMaxPointLights;
using engine::render::MeshRenderer;
using engine::render::pack_lights;
using engine::render::PointLight;

namespace {

MeshRenderer renderer(std::uint32_t mesh, std::uint32_t texture = 0xFFFFFFFFu) {
    MeshRenderer mr;
    mr.mesh = Handle<Mesh>{mesh, 1};
    mr.texture = Handle<Image>{texture, 1};
    return mr;
}

Transform at(const glm::vec3& p) {
    Transform t;
    t.position = p;
    return t;
}

Entity spawn(Registry& r, const MeshRenderer& mr, const glm::vec3& p) {
    const Entity e = r.create();
    r.add<MeshRenderer>(e, mr);
    r.add<Transform>(e, at(p));
    return e;
}

glm::vec3 translation(const glm::mat4& m) {
    return glm::vec3(m[3]);
}

}  // namespace

TEST(DrawList, EmptyRegistryGivesNothing) {
    Registry r;
    EXPECT_TRUE(collect_draw_items(r, 0.5f).empty());
}

TEST(DrawList, SkipsEntitiesWithoutTransformOrMesh) {
    Registry r;
    const Entity no_transform = r.create();
    r.add<MeshRenderer>(no_transform, renderer(0));
    spawn(r, MeshRenderer{}, {0, 0, 0});  // default handle: invalid mesh
    spawn(r, renderer(1), {1, 0, 0});
    const auto items = collect_draw_items(r, 0.0f);
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0].mesh.index, 1u);
}

TEST(DrawList, WithoutPreviousTransformUsesCurrentRegardlessOfAlpha) {
    Registry r;
    spawn(r, renderer(0), {4, 5, 6});
    for (const float alpha : {0.0f, 0.5f, 1.0f}) {
        const auto items = collect_draw_items(r, alpha);
        ASSERT_EQ(items.size(), 1u);
        EXPECT_EQ(translation(items[0].model), glm::vec3(4, 5, 6));
    }
}

TEST(DrawList, InterpolatesBetweenPreviousAndCurrent) {
    Registry r;
    const Entity e = spawn(r, renderer(0), {10, 0, 0});
    r.add<PreviousTransform>(e).value = at({0, 0, 0});

    EXPECT_NEAR(translation(collect_draw_items(r, 0.0f)[0].model).x, 0.0f, 1e-6f);
    EXPECT_NEAR(translation(collect_draw_items(r, 0.25f)[0].model).x, 2.5f, 1e-5f);
    EXPECT_NEAR(translation(collect_draw_items(r, 0.5f)[0].model).x, 5.0f, 1e-5f);
    EXPECT_NEAR(translation(collect_draw_items(r, 1.0f)[0].model).x, 10.0f, 1e-5f);
}

TEST(DrawList, AlphaIsClamped) {
    Registry r;
    const Entity e = spawn(r, renderer(0), {10, 0, 0});
    r.add<PreviousTransform>(e).value = at({0, 0, 0});
    EXPECT_NEAR(translation(collect_draw_items(r, -3.0f)[0].model).x, 0.0f, 1e-6f);
    EXPECT_NEAR(translation(collect_draw_items(r, 7.0f)[0].model).x, 10.0f, 1e-5f);
    EXPECT_NEAR(translation(collect_draw_items(r, std::nanf(""))[0].model).x, 0.0f, 1e-6f);
}

TEST(DrawList, NormalMatrixCompensatesNonUniformScale) {
    Registry r;
    const Entity e = spawn(r, renderer(0), {0, 0, 0});
    r.get<Transform>(e).scale = {2.0f, 1.0f, 1.0f};
    const DrawItem item = collect_draw_items(r, 1.0f)[0];
    EXPECT_NEAR(item.normal_matrix[0][0], 0.5f, 1e-6f);
    EXPECT_NEAR(item.normal_matrix[1][1], 1.0f, 1e-6f);
}

TEST(DrawList, ZeroScaleDoesNotProduceNaNNormals) {
    Registry r;
    const Entity e = spawn(r, renderer(0), {0, 0, 0});
    r.get<Transform>(e).scale = {0.0f, 1.0f, 1.0f};
    EXPECT_EQ(collect_draw_items(r, 1.0f)[0].normal_matrix, glm::mat3(1.0f));
}

TEST(DrawList, SortedByTextureThenMeshThenEntity) {
    Registry r;
    const Entity a = spawn(r, renderer(2, 5), {0, 0, 0});
    const Entity b = spawn(r, renderer(1, 5), {0, 0, 0});
    const Entity c = spawn(r, renderer(3, 1), {0, 0, 0});
    const Entity d = spawn(r, renderer(1, 5), {0, 0, 0});
    const Entity e = spawn(r, renderer(0), {0, 0, 0});  // no texture sorts last
    const auto items = collect_draw_items(r, 0.0f);
    ASSERT_EQ(items.size(), 5u);
    EXPECT_EQ(items[0].entity, c);
    EXPECT_EQ(items[1].entity, b);
    EXPECT_EQ(items[2].entity, d);
    EXPECT_EQ(items[3].entity, a);
    EXPECT_EQ(items[4].entity, e);
}

TEST(DrawList, OrderDoesNotDependOnComponentInsertionOrder) {
    // Same four entities, components added in a different order: the draw order must be equal.
    auto build = [](bool reversed) {
        Registry r;
        std::vector<Entity> es;
        for (int i = 0; i < 4; ++i) {
            es.push_back(r.create());
        }
        std::vector<int> order = {0, 1, 2, 3};
        if (reversed) {
            std::reverse(order.begin(), order.end());
        }
        for (const int i : order) {
            r.add<Transform>(es[static_cast<std::size_t>(i)], at({static_cast<float>(i), 0, 0}));
            r.add<MeshRenderer>(es[static_cast<std::size_t>(i)],
                                renderer(static_cast<std::uint32_t>(i % 2)));
        }
        std::vector<std::uint32_t> result;
        for (const DrawItem& item : collect_draw_items(r, 0.0f)) {
            result.push_back(item.entity.index);
        }
        return result;
    };
    EXPECT_EQ(build(false), build(true));
}

TEST(DrawList, ReusesOutputBuffer) {
    Registry r;
    spawn(r, renderer(0), {0, 0, 0});
    std::vector<DrawItem> out;
    collect_draw_items(r, 0.0f, out);
    collect_draw_items(r, 0.0f, out);
    EXPECT_EQ(out.size(), 1u);
}

// ---- lights ----

TEST(Lights, EmptySceneHasNoLights) {
    Registry r;
    const auto block = pack_lights(r, {0, 0, 0}, 1.0f);
    EXPECT_EQ(block.point_count, 0u);
    EXPECT_EQ(block.directional_color, glm::vec3(0.0f));
}

TEST(Lights, DirectionalIsNormalizedAndScaledByIntensity) {
    Registry r;
    const Entity e = r.create();
    DirectionalLight l;
    l.direction = {0, -3, 0};
    l.color = {1.0f, 0.5f, 0.0f};
    l.intensity = 2.0f;
    l.ambient = {0.2f, 0.2f, 0.2f};
    r.add<DirectionalLight>(e, l);
    const auto block = pack_lights(r, {0, 0, 0}, 1.0f);
    EXPECT_NEAR(glm::length(block.direction), 1.0f, 1e-6f);
    EXPECT_EQ(block.direction, glm::vec3(0, -1, 0));
    EXPECT_EQ(block.directional_color, glm::vec3(2.0f, 1.0f, 0.0f));
    EXPECT_EQ(block.ambient, glm::vec3(0.2f));
}

TEST(Lights, ZeroDirectionFallsBackToDown) {
    Registry r;
    DirectionalLight l;
    l.direction = {0, 0, 0};
    r.add<DirectionalLight>(r.create(), l);
    EXPECT_EQ(pack_lights(r, {0, 0, 0}, 1.0f).direction, glm::vec3(0, -1, 0));
}

TEST(Lights, FirstDirectionalByEntityWins) {
    Registry r;
    DirectionalLight a;
    a.direction = {1, 0, 0};
    DirectionalLight b;
    b.direction = {0, 0, 1};
    // Add the later entity's component first.
    const Entity first = r.create();
    const Entity second = r.create();
    r.add<DirectionalLight>(second, b);
    r.add<DirectionalLight>(first, a);
    EXPECT_EQ(pack_lights(r, {0, 0, 0}, 1.0f).direction, glm::vec3(1, 0, 0));
}

namespace {
Entity point_light(Registry& r, const glm::vec3& p, float intensity = 1.0f, float range = 10.0f) {
    const Entity e = r.create();
    PointLight l;
    l.intensity = intensity;
    l.range = range;
    r.add<PointLight>(e, l);
    r.add<Transform>(e, at(p));
    return e;
}
}  // namespace

TEST(Lights, PointLightUsesTransformAndColorTimesIntensity) {
    Registry r;
    const Entity e = point_light(r, {1, 2, 3}, 2.0f, 7.0f);
    r.get<PointLight>(e).color = {1.0f, 0.5f, 0.25f};
    const auto block = pack_lights(r, {0, 0, 0}, 1.0f);
    ASSERT_EQ(block.point_count, 1u);
    EXPECT_EQ(block.points[0].position, glm::vec3(1, 2, 3));
    EXPECT_EQ(block.points[0].color, glm::vec3(2.0f, 1.0f, 0.5f));
    EXPECT_EQ(block.points[0].range, 7.0f);
}

TEST(Lights, PointLightIsInterpolated) {
    Registry r;
    const Entity e = point_light(r, {10, 0, 0});
    r.add<PreviousTransform>(e).value = at({0, 0, 0});
    const auto block = pack_lights(r, {0, 0, 0}, 0.5f);
    ASSERT_EQ(block.point_count, 1u);
    EXPECT_NEAR(block.points[0].position.x, 5.0f, 1e-5f);
}

TEST(Lights, SkipsLightsWithoutTransformRangeOrColor) {
    Registry r;
    r.add<PointLight>(r.create());  // no Transform
    point_light(r, {0, 0, 0}, 1.0f, 0.0f);  // no range
    point_light(r, {0, 0, 0}, 0.0f, 5.0f);  // black
    EXPECT_EQ(pack_lights(r, {0, 0, 0}, 1.0f).point_count, 0u);
}

TEST(Lights, LimitsToEightClosestToCamera) {
    Registry r;
    // 12 lights at x = 1..12; the camera is at the origin, so x = 1..8 must win, in that order.
    // Created in reverse so insertion order is not the answer.
    for (int x = 12; x >= 1; --x) {
        point_light(r, {static_cast<float>(x), 0, 0});
    }
    const auto block = pack_lights(r, {0, 0, 0}, 1.0f);
    ASSERT_EQ(block.point_count, kMaxPointLights);
    for (std::size_t i = 0; i < kMaxPointLights; ++i) {
        EXPECT_FLOAT_EQ(block.points[i].position.x, static_cast<float>(i + 1));
    }
}

TEST(Lights, EqualDistanceTieBrokenByEntityIndex) {
    Registry r;
    const Entity first = point_light(r, {1, 0, 0}, 1.0f);
    const Entity second = point_light(r, {-1, 0, 0}, 3.0f);
    (void)first;
    (void)second;
    const auto block = pack_lights(r, {0, 0, 0}, 1.0f);
    ASSERT_EQ(block.point_count, 2u);
    EXPECT_FLOAT_EQ(block.points[0].position.x, 1.0f);   // lower entity index first
    EXPECT_FLOAT_EQ(block.points[1].position.x, -1.0f);
}

TEST(Lights, ClosestLightFollowsTheCamera) {
    Registry r;
    for (int x = 0; x < 10; ++x) {
        point_light(r, {static_cast<float>(x) * 10.0f, 0, 0});
    }
    const auto block = pack_lights(r, {90, 0, 0}, 1.0f);
    ASSERT_EQ(block.point_count, kMaxPointLights);
    EXPECT_FLOAT_EQ(block.points[0].position.x, 90.0f);
}
