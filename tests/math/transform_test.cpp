#include <gtest/gtest.h>

#include "engine/math/transform.hpp"

using engine::math::interpolate;
using engine::math::PreviousTransform;
using engine::math::Transform;

namespace {

constexpr float kEps = 1e-6f;

void expect_vec_near(const glm::vec3& a, const glm::vec3& b, float eps) {
    EXPECT_NEAR(a.x, b.x, eps);
    EXPECT_NEAR(a.y, b.y, eps);
    EXPECT_NEAR(a.z, b.z, eps);
}

}  // namespace

TEST(Transform, DefaultIsIdentity) {
    Transform t;
    EXPECT_EQ(t.to_matrix(), glm::mat4(1.0f));
}

TEST(Transform, TranslationOnly) {
    Transform t;
    t.position = {1.0f, 2.0f, 3.0f};
    const glm::vec4 p = t.to_matrix() * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    EXPECT_EQ(p, glm::vec4(2.0f, 3.0f, 4.0f, 1.0f));
}

TEST(Transform, ScaleOnly) {
    Transform t;
    t.scale = {2.0f, 4.0f, 8.0f};
    const glm::vec4 p = t.to_matrix() * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    EXPECT_EQ(p, glm::vec4(2.0f, 4.0f, 8.0f, 1.0f));
}

TEST(Transform, TrsOrderIsScaleThenRotateThenTranslate) {
    Transform t;
    t.position = {10.0f, 0.0f, 0.0f};
    t.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    t.scale = {2.0f, 1.0f, 1.0f};
    // (1,0,0) -> scale -> (2,0,0) -> rotate 90 deg about Y -> (0,0,-2) -> translate -> (10,0,-2)
    const glm::vec4 p = t.to_matrix() * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    expect_vec_near(glm::vec3(p), glm::vec3(10.0f, 0.0f, -2.0f), kEps * 4);
    EXPECT_FLOAT_EQ(p.w, 1.0f);
}

TEST(Interpolate, AlphaZeroGivesPrevAndOneGivesCurr) {
    Transform a;
    a.position = {1.0f, 2.0f, 3.0f};
    a.scale = {1.0f, 2.0f, 4.0f};
    Transform b;
    b.position = {5.0f, 6.0f, 7.0f};
    b.scale = {2.0f, 2.0f, 2.0f};
    b.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    const Transform at0 = interpolate(a, b, 0.0f);
    EXPECT_EQ(at0.position, a.position);
    EXPECT_EQ(at0.scale, a.scale);
    EXPECT_NEAR(glm::dot(at0.rotation, a.rotation), 1.0f, kEps);

    const Transform at1 = interpolate(a, b, 1.0f);
    EXPECT_EQ(at1.position, b.position);
    EXPECT_EQ(at1.scale, b.scale);
    EXPECT_NEAR(std::abs(glm::dot(at1.rotation, b.rotation)), 1.0f, kEps);
}

TEST(Interpolate, HalfwayWithPowersOfTwoIsExact) {
    Transform a;
    a.position = {0.0f, 2.0f, -4.0f};
    a.scale = {1.0f, 1.0f, 1.0f};
    Transform b;
    b.position = {4.0f, 6.0f, 4.0f};
    b.scale = {3.0f, 3.0f, 3.0f};

    const Transform mid = interpolate(a, b, 0.5f);
    EXPECT_EQ(mid.position, glm::vec3(2.0f, 4.0f, 0.0f));
    EXPECT_EQ(mid.scale, glm::vec3(2.0f, 2.0f, 2.0f));
}

TEST(Interpolate, EqualRotationsStayEqual) {
    Transform a;
    a.rotation = glm::angleAxis(glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    Transform b = a;
    const Transform out = interpolate(a, b, 0.37f);
    EXPECT_NEAR(std::abs(glm::dot(out.rotation, a.rotation)), 1.0f, kEps);
}

TEST(Interpolate, SlerpHalfwayBetween0And90DegreesIs45) {
    Transform a;
    Transform b;
    b.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const Transform mid = interpolate(a, b, 0.5f);
    const glm::quat expected = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_NEAR(std::abs(glm::dot(mid.rotation, expected)), 1.0f, 1e-5f);
}

TEST(PreviousTransform, DefaultsToIdentity) {
    PreviousTransform p;
    EXPECT_EQ(p.value.to_matrix(), glm::mat4(1.0f));
}
