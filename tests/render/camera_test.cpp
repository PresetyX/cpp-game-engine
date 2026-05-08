#include <gtest/gtest.h>

#include <cmath>

#include "engine/render/camera.hpp"

using engine::render::OrbitCamera;
using engine::render::PerspectiveCamera;

namespace {
void expect_vec_near(const glm::vec3& a, const glm::vec3& b, float eps = 1e-5f) {
    EXPECT_NEAR(a.x, b.x, eps);
    EXPECT_NEAR(a.y, b.y, eps);
    EXPECT_NEAR(a.z, b.z, eps);
}
}  // namespace

TEST(PerspectiveCamera, ZeroSizeGivesNoProjection) {
    const PerspectiveCamera cam;
    EXPECT_FALSE(cam.projection(0, 600).has_value());
    EXPECT_FALSE(cam.projection(800, 0).has_value());
    EXPECT_FALSE(cam.projection(-1, 10).has_value());
}

TEST(PerspectiveCamera, MatchesGlmPerspective) {
    PerspectiveCamera cam;
    cam.fov_y = glm::radians(90.0f);
    cam.near_plane = 1.0f;
    cam.far_plane = 100.0f;
    const auto p = cam.projection(1600, 900);
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(*p, glm::perspective(glm::radians(90.0f), 1600.0f / 900.0f, 1.0f, 100.0f));
}

TEST(PerspectiveCamera, NearAndFarMapToClipRange) {
    PerspectiveCamera cam;
    cam.near_plane = 0.5f;
    cam.far_plane = 50.0f;
    const glm::mat4 p = *cam.projection(100, 100);
    const glm::vec4 n = p * glm::vec4(0.0f, 0.0f, -0.5f, 1.0f);
    const glm::vec4 f = p * glm::vec4(0.0f, 0.0f, -50.0f, 1.0f);
    EXPECT_NEAR(n.z / n.w, -1.0f, 1e-5f);
    EXPECT_NEAR(f.z / f.w, 1.0f, 1e-4f);
}

TEST(PerspectiveCamera, WiderWindowNarrowsX) {
    const PerspectiveCamera cam;
    const glm::mat4 square = *cam.projection(100, 100);
    const glm::mat4 wide = *cam.projection(200, 100);
    EXPECT_NEAR(wide[0][0], square[0][0] / 2.0f, 1e-6f);
    EXPECT_FLOAT_EQ(wide[1][1], square[1][1]);
}

TEST(OrbitCamera, EyeAtYawZeroPitchZeroIsPlusZ) {
    OrbitCamera cam;
    cam.target = {1.0f, 2.0f, 3.0f};
    cam.distance = 5.0f;
    cam.yaw = 0.0f;
    cam.pitch = 0.0f;
    expect_vec_near(cam.eye(), {1.0f, 2.0f, 8.0f});
}

TEST(OrbitCamera, EyeStaysAtDistanceFromTarget) {
    OrbitCamera cam;
    cam.target = {-2.0f, 0.5f, 4.0f};
    cam.distance = 7.5f;
    for (float yaw = -3.0f; yaw < 3.0f; yaw += 0.7f) {
        for (float pitch = -1.4f; pitch < 1.4f; pitch += 0.5f) {
            cam.yaw = yaw;
            cam.pitch = pitch;
            EXPECT_NEAR(glm::length(cam.eye() - cam.target), 7.5f, 1e-4f);
        }
    }
}

TEST(OrbitCamera, PositivePitchMovesEyeUp) {
    OrbitCamera cam;
    cam.pitch = glm::radians(45.0f);
    EXPECT_GT(cam.eye().y, cam.target.y);
}

TEST(OrbitCamera, PitchIsClamped) {
    OrbitCamera cam;
    cam.pitch = 0.0f;
    cam.orbit(0.0f, 100.0f);
    EXPECT_FLOAT_EQ(cam.pitch, OrbitCamera::max_pitch);
    cam.orbit(0.0f, -1000.0f);
    EXPECT_FLOAT_EQ(cam.pitch, -OrbitCamera::max_pitch);
    EXPECT_TRUE(std::isfinite(cam.view()[0][0]));
}

TEST(OrbitCamera, YawWrapsAndOrbitAccumulates) {
    OrbitCamera cam;
    cam.yaw = 0.0f;
    cam.orbit(glm::two_pi<float>() + 0.5f, 0.0f);
    EXPECT_NEAR(cam.yaw, 0.5f, 1e-5f);
}

TEST(OrbitCamera, ZoomClampsDistance) {
    OrbitCamera cam;
    cam.distance = 10.0f;
    cam.zoom(0.5f);
    EXPECT_FLOAT_EQ(cam.distance, 5.0f);
    cam.zoom(1e-9f);
    EXPECT_FLOAT_EQ(cam.distance, OrbitCamera::min_distance);
    cam.zoom(1e12f);
    EXPECT_FLOAT_EQ(cam.distance, OrbitCamera::max_distance);
}

TEST(OrbitCamera, ViewMatchesLookAt) {
    OrbitCamera cam;
    cam.target = {1.0f, 1.0f, 1.0f};
    cam.yaw = 0.8f;
    cam.pitch = 0.3f;
    cam.distance = 6.0f;
    EXPECT_EQ(cam.view(), glm::lookAt(cam.eye(), cam.target, glm::vec3(0, 1, 0)));
}

TEST(OrbitCamera, TargetIsInFrontOfCameraOnAxis) {
    OrbitCamera cam;
    cam.target = {3.0f, 0.0f, -2.0f};
    cam.distance = 4.0f;
    cam.yaw = 1.1f;
    cam.pitch = 0.4f;
    const glm::vec4 t = cam.view() * glm::vec4(cam.target, 1.0f);
    EXPECT_NEAR(t.x, 0.0f, 1e-5f);
    EXPECT_NEAR(t.y, 0.0f, 1e-5f);
    EXPECT_NEAR(t.z, -4.0f, 1e-5f);
}

TEST(OrbitCamera, PanMovesEyeAndTargetTogether) {
    OrbitCamera cam;
    const glm::vec3 offset = cam.eye() - cam.target;
    cam.pan({2.0f, 0.0f, -1.0f});
    expect_vec_near(cam.eye() - cam.target, offset);
    expect_vec_near(cam.target, {2.0f, 0.0f, -1.0f});
}
