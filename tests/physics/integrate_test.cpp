#include <gtest/gtest.h>

#include "engine/ecs/registry.hpp"
#include "engine/math/transform.hpp"
#include "engine/physics/components.hpp"
#include "engine/physics/integrate.hpp"

using engine::Registry;
using engine::math::PreviousTransform;
using engine::math::Transform;
using engine::physics::Collider;
using engine::physics::RigidBody;
using engine::physics::integrate_bodies;
using engine::physics::store_previous_transforms;

namespace {

const glm::vec3 kGravity{0.0f, -10.0f, 0.0f};

}  // namespace

TEST(Integrate, SemiImplicitEulerUsesUpdatedVelocity) {
    Registry r;
    const auto e = r.create();
    r.add<Transform>(e);
    r.add<RigidBody>(e);

    integrate_bodies(r, 0.1f, kGravity);

    // v = -1.0 after the step, and position uses that new v: -0.1 (explicit Euler would give 0).
    EXPECT_NEAR(r.get<RigidBody>(e).velocity.y, -1.0f, 1e-6f);
    EXPECT_NEAR(r.get<Transform>(e).position.y, -0.1f, 1e-6f);
}

TEST(Integrate, FreeFallMatchesSemiImplicitClosedForm) {
    Registry r;
    const auto e = r.create();
    r.add<Transform>(e);
    r.add<RigidBody>(e);

    const float dt = 1.0f / 60.0f;
    const int n = 60;
    for (int i = 0; i < n; ++i) integrate_bodies(r, dt, kGravity);

    // y_n = g * dt^2 * n(n+1)/2
    const float expected = -10.0f * dt * dt * n * (n + 1) / 2.0f;
    EXPECT_NEAR(r.get<Transform>(e).position.y, expected, 1e-3f);
}

TEST(Integrate, ForceIsAppliedAsAccelerationAndClearedAfterStep) {
    Registry r;
    const auto e = r.create();
    r.add<Transform>(e);
    auto& body = r.add<RigidBody>(e, RigidBody::with_mass(2.0f));
    body.add_force({4.0f, 0.0f, 0.0f});

    integrate_bodies(r, 0.5f, glm::vec3(0.0f));

    EXPECT_NEAR(r.get<RigidBody>(e).velocity.x, 1.0f, 1e-6f);  // a = 4/2, v = a*dt
    EXPECT_EQ(r.get<RigidBody>(e).force, glm::vec3(0.0f));

    integrate_bodies(r, 0.5f, glm::vec3(0.0f));
    EXPECT_NEAR(r.get<RigidBody>(e).velocity.x, 1.0f, 1e-6f);  // no force left
}

TEST(Integrate, GravityScaleScalesGravity) {
    Registry r;
    const auto e = r.create();
    r.add<Transform>(e);
    RigidBody b;
    b.gravity_scale = 0.0f;
    r.add<RigidBody>(e, b);

    integrate_bodies(r, 1.0f, kGravity);
    EXPECT_EQ(r.get<RigidBody>(e).velocity, glm::vec3(0.0f));
}

TEST(Integrate, KinematicIgnoresGravityAndForcesButMoves) {
    Registry r;
    const auto e = r.create();
    r.add<Transform>(e);
    auto& body = r.add<RigidBody>(e, RigidBody::kinematic());
    body.velocity = {2.0f, 0.0f, 0.0f};
    body.add_force({100.0f, 100.0f, 100.0f});

    integrate_bodies(r, 0.5f, kGravity);

    EXPECT_EQ(r.get<RigidBody>(e).velocity, glm::vec3(2.0f, 0.0f, 0.0f));
    EXPECT_EQ(r.get<Transform>(e).position, glm::vec3(1.0f, 0.0f, 0.0f));
}

TEST(Integrate, DampingReducesVelocityAndIsStableForHugeDt) {
    Registry r;
    const auto e = r.create();
    r.add<Transform>(e);
    RigidBody b;
    b.velocity = {10.0f, 0.0f, 0.0f};
    b.linear_damping = 1.0f;
    b.gravity_scale = 0.0f;
    r.add<RigidBody>(e, b);

    integrate_bodies(r, 1.0f, glm::vec3(0.0f));
    EXPECT_NEAR(r.get<RigidBody>(e).velocity.x, 5.0f, 1e-6f);  // 10 / (1 + 1*1)

    integrate_bodies(r, 1000.0f, glm::vec3(0.0f));
    const float v = r.get<RigidBody>(e).velocity.x;
    EXPECT_GE(v, 0.0f);
    EXPECT_LT(v, 0.01f);
}

TEST(Integrate, StaticEntitiesWithoutRigidBodyDoNotMove) {
    Registry r;
    const auto e = r.create();
    r.add<Transform>(e).position = {1.0f, 2.0f, 3.0f};
    r.add<Collider>(e, Collider::box({1.0f, 1.0f, 1.0f}));

    integrate_bodies(r, 1.0f, kGravity);
    EXPECT_EQ(r.get<Transform>(e).position, glm::vec3(1.0f, 2.0f, 3.0f));
}

TEST(Integrate, BodyWithoutTransformIsIgnored) {
    Registry r;
    const auto e = r.create();
    r.add<RigidBody>(e);

    integrate_bodies(r, 1.0f, kGravity);
    EXPECT_EQ(r.get<RigidBody>(e).velocity, glm::vec3(0.0f));
}

TEST(Integrate, ImpulseChangesVelocityByInverseMass) {
    RigidBody b = RigidBody::with_mass(4.0f);
    b.add_impulse({8.0f, 0.0f, 0.0f});
    EXPECT_NEAR(b.velocity.x, 2.0f, 1e-6f);

    RigidBody k = RigidBody::kinematic();
    k.add_impulse({8.0f, 0.0f, 0.0f});
    EXPECT_EQ(k.velocity, glm::vec3(0.0f));
}

TEST(PreviousTransform, StoreCopiesCurrentBeforeMovement) {
    Registry r;
    const auto e = r.create();
    r.add<Transform>(e).position = {1.0f, 0.0f, 0.0f};
    r.add<PreviousTransform>(e);
    auto& body = r.add<RigidBody>(e);
    body.velocity = {1.0f, 0.0f, 0.0f};
    body.gravity_scale = 0.0f;

    store_previous_transforms(r);
    integrate_bodies(r, 1.0f, glm::vec3(0.0f));

    EXPECT_EQ(r.get<PreviousTransform>(e).value.position, glm::vec3(1.0f, 0.0f, 0.0f));
    EXPECT_EQ(r.get<Transform>(e).position, glm::vec3(2.0f, 0.0f, 0.0f));
}

TEST(PreviousTransform, EntitiesWithoutItAreSkipped) {
    Registry r;
    const auto e = r.create();
    r.add<Transform>(e);
    store_previous_transforms(r);
    EXPECT_FALSE(r.has<PreviousTransform>(e));
}

TEST(Components, ColliderFactories) {
    const Collider s = Collider::sphere(2.0f, 0.3f, 0.7f);
    EXPECT_EQ(s.type, engine::physics::ShapeType::Sphere);
    EXPECT_EQ(s.radius, 2.0f);
    EXPECT_EQ(s.friction, 0.3f);
    EXPECT_EQ(s.restitution, 0.7f);

    const Collider b = Collider::box({1.0f, 2.0f, 3.0f});
    EXPECT_EQ(b.type, engine::physics::ShapeType::Box);
    EXPECT_EQ(b.half_extents, glm::vec3(1.0f, 2.0f, 3.0f));
}
