#include <gtest/gtest.h>

#include <cmath>

#include "engine/physics/narrowphase.hpp"

using engine::physics::collide;
using engine::physics::collide_box_box;
using engine::physics::collide_sphere_box;
using engine::physics::collide_sphere_sphere;
using engine::physics::Collider;
using glm::vec3;

namespace {
constexpr float kTol = 1e-5f;

void expect_vec(const vec3& a, const vec3& b) {
    EXPECT_NEAR(a.x, b.x, kTol);
    EXPECT_NEAR(a.y, b.y, kTol);
    EXPECT_NEAR(a.z, b.z, kTol);
}
}  // namespace

// ---------- sphere-sphere ----------
TEST(SphereSphere, Separated) {
    EXPECT_FALSE(collide_sphere_sphere({0, 0, 0}, 1, {3, 0, 0}, 1));
}

TEST(SphereSphere, ExactTouchIsNotContact) {
    EXPECT_FALSE(collide_sphere_sphere({0, 0, 0}, 1, {2, 0, 0}, 1));
}

TEST(SphereSphere, OverlapAlongX) {
    auto c = collide_sphere_sphere({0, 0, 0}, 1, {1.5f, 0, 0}, 1);
    ASSERT_TRUE(c);
    expect_vec(c->normal, {1, 0, 0});
    EXPECT_NEAR(c->depth, 0.5f, kTol);
    expect_vec(c->point, {0.75f, 0, 0});
}

TEST(SphereSphere, DiagonalNormalIsUnit) {
    auto c = collide_sphere_sphere({0, 0, 0}, 1, {1, 1, 1}, 1);
    ASSERT_TRUE(c);
    EXPECT_NEAR(glm::length(c->normal), 1.0f, kTol);
    expect_vec(c->normal, glm::normalize(vec3{1, 1, 1}));
    EXPECT_GT(c->depth, 0.0f);
}

TEST(SphereSphere, CoincidentCentersDeterministic) {
    auto c1 = collide_sphere_sphere({2, 3, 4}, 1, {2, 3, 4}, 0.5f);
    auto c2 = collide_sphere_sphere({2, 3, 4}, 1, {2, 3, 4}, 0.5f);
    ASSERT_TRUE(c1);
    ASSERT_TRUE(c2);
    expect_vec(c1->normal, {0, 1, 0});
    EXPECT_NEAR(c1->depth, 1.5f, kTol);
    expect_vec(c1->normal, c2->normal);
    EXPECT_FLOAT_EQ(c1->depth, c2->depth);
    // Swapping still yields +Y (documented degenerate exception).
    auto s = collide_sphere_sphere({2, 3, 4}, 0.5f, {2, 3, 4}, 1);
    ASSERT_TRUE(s);
    expect_vec(s->normal, {0, 1, 0});
}

TEST(SphereSphere, SwapFlipsNormal) {
    auto ab = collide_sphere_sphere({0, 0, 0}, 1, {1, 0.5f, 0}, 0.75f);
    auto ba = collide_sphere_sphere({1, 0.5f, 0}, 0.75f, {0, 0, 0}, 1);
    ASSERT_TRUE(ab);
    ASSERT_TRUE(ba);
    expect_vec(ab->normal, -ba->normal);
    EXPECT_NEAR(ab->depth, ba->depth, kTol);
    expect_vec(ab->point, ba->point);
}

TEST(SphereSphere, OneContainsOther) {
    auto c = collide_sphere_sphere({0, 0, 0}, 2, {0.5f, 0, 0}, 0.5f);
    ASSERT_TRUE(c);
    expect_vec(c->normal, {1, 0, 0});
    EXPECT_NEAR(c->depth, 2.0f, kTol);
}

// ---------- sphere-box ----------
TEST(SphereBox, Separated) {
    EXPECT_FALSE(collide_sphere_box({3, 0, 0}, 1, {0, 0, 0}, {1, 1, 1}));
}

TEST(SphereBox, ExactTouchOnFaceIsNotContact) {
    EXPECT_FALSE(collide_sphere_box({2, 0, 0}, 1, {0, 0, 0}, {1, 1, 1}));
}

TEST(SphereBox, FaceOverlap) {
    auto c = collide_sphere_box({1.5f, 0, 0}, 1, {0, 0, 0}, {1, 1, 1});
    ASSERT_TRUE(c);
    expect_vec(c->normal, {-1, 0, 0});
    EXPECT_NEAR(c->depth, 0.5f, kTol);
    expect_vec(c->point, {0.75f, 0, 0});
}

TEST(SphereBox, EdgeOverlap) {
    auto c = collide_sphere_box({1.5f, 1.5f, 0}, 1, {0, 0, 0}, {1, 1, 1});
    ASSERT_TRUE(c);
    expect_vec(c->normal, glm::normalize(vec3{-1, -1, 0}));
    EXPECT_NEAR(c->depth, 1.0f - std::sqrt(0.5f), kTol);
}

TEST(SphereBox, VertexOverlap) {
    auto c = collide_sphere_box({1.5f, 1.5f, 1.5f}, 1, {0, 0, 0}, {1, 1, 1});
    ASSERT_TRUE(c);
    expect_vec(c->normal, glm::normalize(vec3{-1, -1, -1}));
    EXPECT_NEAR(c->depth, 1.0f - std::sqrt(0.75f), kTol);
}

TEST(SphereBox, CornerInsideExpandedBoxButOutsideRadius) {
    EXPECT_FALSE(collide_sphere_box({1.9f, 1.9f, 1.9f}, 1, {0, 0, 0}, {1, 1, 1}));
}

TEST(SphereBox, CenterExactlyOnFaceSurface) {
    auto c = collide_sphere_box({1, 0.25f, -0.25f}, 0.5f, {0, 0, 0}, {1, 1, 1});
    ASSERT_TRUE(c);
    expect_vec(c->normal, {-1, 0, 0});
    EXPECT_NEAR(c->depth, 0.5f, kTol);  // == radius
    expect_vec(c->point, {0.75f, 0.25f, -0.25f});
}

TEST(SphereBox, CenterExactlyOnEdgeSurface) {
    // On the edge: x+ and y+ tie, resolved by order (x before y).
    auto c = collide_sphere_box({1, 1, 0}, 0.5f, {0, 0, 0}, {1, 1, 1});
    ASSERT_TRUE(c);
    expect_vec(c->normal, {-1, 0, 0});
    EXPECT_NEAR(c->depth, 0.5f, kTol);
}

TEST(SphereBox, CenterInsideNearPlusX) {
    auto c = collide_sphere_box({0.8f, 0, 0}, 0.5f, {0, 0, 0}, {1, 1, 1});
    ASSERT_TRUE(c);
    expect_vec(c->normal, {-1, 0, 0});
    EXPECT_NEAR(c->depth, 0.7f, kTol);  // r + distance to face
}

TEST(SphereBox, CenterInsideNearMinusY) {
    auto c = collide_sphere_box({0, -0.9f, 0}, 0.25f, {0, 0, 0}, {1, 1, 1});
    ASSERT_TRUE(c);
    expect_vec(c->normal, {0, 1, 0});
    EXPECT_NEAR(c->depth, 0.35f, kTol);
}

TEST(SphereBox, CenterAtBoxCenterTieGoesToMinusXFace) {
    auto c = collide_sphere_box({0, 0, 0}, 0.5f, {0, 0, 0}, {1, 1, 1});
    ASSERT_TRUE(c);
    // The tie-break picks the x- face, so the sphere is pushed out through it: n = +x.
    expect_vec(c->normal, {1, 0, 0});
    EXPECT_NEAR(c->depth, 1.5f, kTol);
    auto c2 = collide_sphere_box({0, 0, 0}, 0.5f, {0, 0, 0}, {1, 1, 1});
    ASSERT_TRUE(c2);
    expect_vec(c->normal, c2->normal);
}

TEST(SphereBox, FlatBoxPicksThinnestAxis) {
    auto c = collide_sphere_box({0.5f, 0.05f, 0.5f}, 0.2f, {0, 0, 0}, {5, 0.1f, 5});
    ASSERT_TRUE(c);
    expect_vec(c->normal, {0, -1, 0});
    EXPECT_NEAR(c->depth, 0.2f + 0.05f, kTol);
}

TEST(SphereBox, SphereOnTopOfGround) {
    auto c = collide_sphere_box({0, 0.4f, 0}, 0.5f, {0, -0.5f, 0}, {10, 0.5f, 10});
    ASSERT_TRUE(c);
    expect_vec(c->normal, {0, -1, 0});
    EXPECT_NEAR(c->depth, 0.1f, kTol);
}

// ---------- box-box ----------
TEST(BoxBox, SeparatedOnOneAxisOnly) {
    EXPECT_FALSE(collide_box_box({0, 0, 0}, {1, 1, 1}, {0.5f, 3, 0.5f}, {1, 1, 1}));
}

TEST(BoxBox, ExactTouchOnFaceIsNotContact) {
    EXPECT_FALSE(collide_box_box({0, 0, 0}, {1, 1, 1}, {2, 0, 0}, {1, 1, 1}));
}

TEST(BoxBox, SmallestOverlapAxisY) {
    auto c = collide_box_box({0, 0, 0}, {1, 1, 1}, {0.5f, 1.8f, 0.25f}, {1, 1, 1});
    ASSERT_TRUE(c);
    expect_vec(c->normal, {0, 1, 0});
    EXPECT_NEAR(c->depth, 0.2f, kTol);
}

TEST(BoxBox, NegativeDirectionNormal) {
    auto c = collide_box_box({0, 0, 0}, {1, 1, 1}, {-1.5f, 0, 0}, {1, 1, 1});
    ASSERT_TRUE(c);
    expect_vec(c->normal, {-1, 0, 0});
    EXPECT_NEAR(c->depth, 0.5f, kTol);
}

TEST(BoxBox, TieXYPicksX) {
    auto c = collide_box_box({0, 0, 0}, {1, 1, 1}, {1.5f, 1.5f, 0}, {1, 1, 1});
    ASSERT_TRUE(c);
    expect_vec(c->normal, {1, 0, 0});
    EXPECT_NEAR(c->depth, 0.5f, kTol);
}

TEST(BoxBox, CoincidentCentersDeterministic) {
    auto c = collide_box_box({1, 2, 3}, {1, 2, 3}, {1, 2, 3}, {0.5f, 0.5f, 0.5f});
    ASSERT_TRUE(c);
    expect_vec(c->normal, {1, 0, 0});
    EXPECT_NEAR(c->depth, 1.5f, kTol);
    auto s = collide_box_box({1, 2, 3}, {0.5f, 0.5f, 0.5f}, {1, 2, 3}, {1, 2, 3});
    ASSERT_TRUE(s);
    expect_vec(s->normal, {1, 0, 0});  // documented degenerate exception
}

TEST(BoxBox, PointIsCenterOfIntersection) {
    auto c = collide_box_box({0, 0, 0}, {1, 1, 1}, {1.5f, 0.5f, 0}, {1, 1, 1});
    ASSERT_TRUE(c);
    expect_vec(c->point, {0.75f, 0.25f, 0});
}

TEST(BoxBox, SwapFlipsNormal) {
    auto ab = collide_box_box({0, 0, 0}, {1, 1, 1}, {1.2f, 0.2f, 0.3f}, {0.5f, 1, 0.75f});
    auto ba = collide_box_box({1.2f, 0.2f, 0.3f}, {0.5f, 1, 0.75f}, {0, 0, 0}, {1, 1, 1});
    ASSERT_TRUE(ab);
    ASSERT_TRUE(ba);
    expect_vec(ab->normal, -ba->normal);
    EXPECT_NEAR(ab->depth, ba->depth, kTol);
    expect_vec(ab->point, ba->point);
}

TEST(BoxBox, SmallBoxInsideLarge) {
    auto c = collide_box_box({0, 0, 0}, {5, 5, 5}, {1, 0, 0}, {0.5f, 0.5f, 0.5f});
    ASSERT_TRUE(c);
    expect_vec(c->normal, {1, 0, 0});
    EXPECT_NEAR(c->depth, 4.5f, kTol);  // 5 + 0.5 - 1
}

// ---------- dispatch ----------
TEST(Dispatch, AllShapePairs) {
    const Collider s = Collider::sphere(1.0f);
    const Collider b = Collider::box({1, 1, 1});
    EXPECT_TRUE(collide(s, {0, 0, 0}, s, {1, 0, 0}));
    EXPECT_TRUE(collide(s, {0, 0, 0}, b, {1.5f, 0, 0}));
    EXPECT_TRUE(collide(b, {0, 0, 0}, s, {1.5f, 0, 0}));
    EXPECT_TRUE(collide(b, {0, 0, 0}, b, {1, 0, 0}));
}

TEST(Dispatch, BoxSphereIsNegatedSphereBox) {
    const Collider s = Collider::sphere(1.0f);
    const Collider b = Collider::box({1, 1, 1});
    auto sb = collide(s, {1.5f, 0.3f, 0}, b, {0, 0, 0});
    auto bs = collide(b, {0, 0, 0}, s, {1.5f, 0.3f, 0});
    ASSERT_TRUE(sb);
    ASSERT_TRUE(bs);
    expect_vec(bs->normal, -sb->normal);
    EXPECT_FLOAT_EQ(bs->depth, sb->depth);
    expect_vec(bs->point, sb->point);
}

TEST(Dispatch, ReadsColliderFields) {
    const Collider s = Collider::sphere(2.0f);
    const Collider b = Collider::box({0.5f, 1.0f, 3.0f});
    EXPECT_FALSE(collide(s, {0, 0, 0}, b, {2.5f, 0, 0}));  // touching: 2 + 0.5
    EXPECT_TRUE(collide(s, {0, 0, 0}, b, {2.4f, 0, 0}));
    EXPECT_FALSE(collide(b, {0, 0, 0}, b, {1.0f, 0, 0}));  // x touch: 0.5 + 0.5
    EXPECT_TRUE(collide(b, {0, 0, 0}, b, {0, 1.9f, 0}));
}
