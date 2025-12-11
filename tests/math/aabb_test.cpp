#include <gtest/gtest.h>

#include "engine/math/aabb.hpp"

using engine::math::Aabb;
using engine::math::contains;
using engine::math::overlaps;

namespace {
Aabb box(float x0, float y0, float z0, float x1, float y1, float z1) {
    return Aabb{{x0, y0, z0}, {x1, y1, z1}};
}
}  // namespace

TEST(Aabb, SeparatedDoNotOverlap) {
    EXPECT_FALSE(overlaps(box(0, 0, 0, 1, 1, 1), box(2, 0, 0, 3, 1, 1)));
}

TEST(Aabb, TouchingOnAFaceCountsAsOverlap) {
    EXPECT_TRUE(overlaps(box(0, 0, 0, 1, 1, 1), box(1, 0, 0, 2, 1, 1)));
}

TEST(Aabb, OverlapOnlyInOneAxisIsNotOverlap) {
    // Overlap in x and y, separated in z.
    EXPECT_FALSE(overlaps(box(0, 0, 0, 2, 2, 1), box(1, 1, 2, 3, 3, 3)));
}

TEST(Aabb, OverlappingBoxes) {
    EXPECT_TRUE(overlaps(box(0, 0, 0, 2, 2, 2), box(1, 1, 1, 3, 3, 3)));
}

TEST(Aabb, OverlapIsSymmetric) {
    const Aabb a = box(0, 0, 0, 2, 2, 2);
    const Aabb b = box(1, 1, 1, 3, 3, 3);
    EXPECT_EQ(overlaps(a, b), overlaps(b, a));
}

TEST(Aabb, Contains) {
    const Aabb outer = box(0, 0, 0, 4, 4, 4);
    EXPECT_TRUE(contains(outer, box(1, 1, 1, 2, 2, 2)));
    EXPECT_TRUE(contains(outer, outer));
    EXPECT_FALSE(contains(outer, box(3, 3, 3, 5, 5, 5)));
    EXPECT_FALSE(contains(box(1, 1, 1, 2, 2, 2), outer));
}

TEST(Aabb, FromCenterHalfExtentsRoundTrip) {
    const Aabb a = Aabb::from_center_half_extents({1.0f, 2.0f, 3.0f}, {0.5f, 1.0f, 2.0f});
    EXPECT_EQ(a.min, glm::vec3(0.5f, 1.0f, 1.0f));
    EXPECT_EQ(a.max, glm::vec3(1.5f, 3.0f, 5.0f));
    EXPECT_EQ(a.center(), glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(a.half_extents(), glm::vec3(0.5f, 1.0f, 2.0f));
}
