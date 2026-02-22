#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "engine/ecs/registry.hpp"
#include "engine/math/transform.hpp"
#include "engine/physics/components.hpp"
#include "engine/physics/world.hpp"

using engine::Entity;
using engine::Registry;
using engine::math::PreviousTransform;
using engine::math::Transform;
using engine::physics::Collider;
using engine::physics::PhysicsSettings;
using engine::physics::PhysicsWorld;
using engine::physics::RigidBody;

namespace {

constexpr float kDt = 1.0f / 60.0f;

Entity add_static(Registry& r, const Collider& c, const glm::vec3& pos) {
    const Entity e = r.create();
    r.add<Transform>(e).position = pos;
    r.add<Collider>(e, c);
    return e;
}

Entity add_dynamic(Registry& r, const Collider& c, const glm::vec3& pos, float mass,
                   const glm::vec3& vel = glm::vec3(0.0f)) {
    const Entity e = add_static(r, c, pos);
    RigidBody& b = r.add<RigidBody>(e, RigidBody::with_mass(mass));
    b.velocity = vel;
    return e;
}

glm::vec3 pos(const Registry& r, Entity e) {
    return r.get<Transform>(e).position;
}
glm::vec3 vel(const Registry& r, Entity e) {
    return r.get<RigidBody>(e).velocity;
}

PhysicsSettings no_gravity() {
    PhysicsSettings s;
    s.gravity = glm::vec3(0.0f);
    return s;
}

}  // namespace

TEST(World, BallRestsOnGround) {
    Registry r;
    PhysicsWorld world;
    add_static(r, Collider::box({10.0f, 0.5f, 10.0f}), {0.0f, -0.5f, 0.0f});  // top at y = 0
    const Entity ball = add_dynamic(r, Collider::sphere(0.5f), {0.0f, 0.5f, 0.0f}, 1.0f);

    for (int i = 0; i < 300; ++i) {
        world.step(r, kDt);
    }

    // vy: after the last solve only rounding is left (measured exactly 0); 1e-4 m/s is far below
    // one step of gravity (g*dt = 0.16 m/s), so any real sinking or bouncing would fail.
    EXPECT_NEAR(vel(r, ball).y, 0.0f, 1e-4f);
    // Resting depth fixed point: slop + g*dt^2*(1/percent - 1) = 0.01 + 0.0041 = 0.0141 m
    // (architect's analytic estimate; measured 0.0141). 1e-3 m is ~7% of that depth.
    EXPECT_NEAR(pos(r, ball).y, 0.5f - 0.0141f, 1e-3f);
    EXPECT_EQ(pos(r, ball).x, 0.0f);
    EXPECT_EQ(pos(r, ball).z, 0.0f);
}

TEST(World, ElasticCollisionSwapsVelocities) {
    Registry r;
    PhysicsWorld world(no_gravity());
    const Collider c = Collider::sphere(0.5f, 0.0f, 1.0f);
    const Entity a = add_dynamic(r, c, {0.0f, 0.0f, 0.0f}, 2.0f, {3.0f, 0.0f, 0.0f});
    const Entity b = add_dynamic(r, c, {0.99f, 0.0f, 0.0f}, 2.0f);

    world.step(r, kDt);

    // Equal masses, e = 1, closing speed 3 > threshold: the first pass swaps the velocities
    // exactly (measured 0 and 3). 1e-5 is ~40 ulp at magnitude 3, only a rounding margin.
    EXPECT_NEAR(vel(r, a).x, 0.0f, 1e-5f);
    EXPECT_NEAR(vel(r, b).x, 3.0f, 1e-5f);
    ASSERT_EQ(world.contacts().size(), 1u);
    EXPECT_FALSE(world.contacts()[0].trigger);
}

TEST(World, MomentumIsConserved) {
    Registry r;
    PhysicsWorld world(no_gravity());
    const Collider c = Collider::sphere(0.5f, 0.3f, 0.4f);
    std::vector<Entity> es;
    const float masses[] = {1.0f, 2.0f, 4.0f, 0.5f};
    es.push_back(add_dynamic(r, c, {0.0f, 0.0f, 0.0f}, masses[0], {4.0f, 0.5f, 0.0f}));
    es.push_back(add_dynamic(r, c, {0.9f, 0.1f, 0.0f}, masses[1], {-1.0f, 0.0f, 0.3f}));
    es.push_back(add_dynamic(r, c, {1.8f, 0.0f, 0.1f}, masses[2], {0.0f, -2.0f, 0.0f}));
    es.push_back(add_dynamic(r, c, {0.9f, 0.9f, 0.0f}, masses[3], {0.0f, 0.0f, 2.0f}));

    const auto momentum = [&] {
        glm::vec3 p(0.0f);
        for (std::size_t i = 0; i < es.size(); ++i) {
            p += vel(r, es[i]) * masses[i];
        }
        return p;
    };
    const glm::vec3 before = momentum();
    bool collided = false;
    for (int i = 0; i < 10; ++i) {
        world.step(r, kDt);
        collided = collided || !world.contacts().empty();
    }
    const glm::vec3 after = momentum();
    ASSERT_TRUE(collided);

    // Impulses are equal and opposite, so only rounding breaks conservation. Measured drift
    // 4.8e-7 (about half an ulp of the ~8 kg m/s total); 1e-5 leaves a 20x margin.
    EXPECT_NEAR(after.x, before.x, 1e-5f);
    EXPECT_NEAR(after.y, before.y, 1e-5f);
    EXPECT_NEAR(after.z, before.z, 1e-5f);
}

TEST(World, FrictionStopsSlidingBody) {
    Registry r;
    PhysicsWorld world;
    constexpr float kV0 = 5.0f;
    add_static(r, Collider::box({50.0f, 0.5f, 50.0f}, 0.5f), {0.0f, -0.5f, 0.0f});
    const Entity box =
        add_dynamic(r, Collider::box({0.5f, 0.5f, 0.5f}, 0.5f), {0.0f, 0.5f, 0.0f}, 1.0f,
                    {kV0, 0.0f, 0.0f});

    for (int i = 0; i < 180; ++i) {
        world.step(r, kDt);
    }

    // Static friction holds once the speed is below mu*g*dt: only rounding is left (measured 0).
    EXPECT_NEAR(vel(r, box).x, 0.0f, 1e-5f);
    // Continuous stopping distance v^2/(2 mu g) = 2.548 m. Semi-implicit stepping plus a first
    // contact-free step travel up to one extra step (v0*dt = 0.083 m); measured 2.590.
    // Accept [continuous - 0.01, continuous + v0*dt + 0.01].
    constexpr float kContinuous = kV0 * kV0 / (2.0f * 0.5f * 9.81f);
    EXPECT_NEAR(pos(r, box).x, kContinuous + 0.5f * kV0 * kDt, 0.5f * kV0 * kDt + 0.01f);
}

TEST(World, TriggerGivesNoPhysicalResponse) {
    const auto run = [](bool with_trigger, std::vector<engine::physics::ContactPair>* out) {
        Registry r;
        PhysicsWorld world(no_gravity());
        const Entity ball = add_dynamic(r, Collider::sphere(0.5f), {-3.0f, 0.0f, 0.0f}, 1.0f,
                                        {6.0f, 0.0f, 0.0f});
        if (with_trigger) {
            Collider zone = Collider::box({1.0f, 1.0f, 1.0f});
            zone.is_trigger = true;
            add_static(r, zone, {0.0f, 0.0f, 0.0f});
        }
        bool saw_overlap = false;
        for (int i = 0; i < 60; ++i) {
            world.step(r, kDt);
            if (!world.contacts().empty()) {
                saw_overlap = true;
                if (out != nullptr && out->empty()) {
                    *out = world.contacts();
                }
            }
        }
        EXPECT_EQ(saw_overlap, with_trigger);
        return std::make_pair(pos(r, ball), vel(r, ball));
    };

    std::vector<engine::physics::ContactPair> reported;
    const auto with = run(true, &reported);
    const auto without = run(false, nullptr);

    // Exact comparison: the trigger must not touch the body at all.
    EXPECT_EQ(with.first, without.first);
    EXPECT_EQ(with.second, without.second);
    ASSERT_EQ(reported.size(), 1u);
    EXPECT_TRUE(reported[0].trigger);
}

TEST(World, KinematicBodyIsDetectedInsideStaticTrigger) {
    Registry r;
    PhysicsWorld world(no_gravity());
    Collider zone = Collider::box({1.0f, 1.0f, 1.0f});
    zone.is_trigger = true;
    const Entity z = add_static(r, zone, {0.0f, 0.0f, 0.0f});
    const Entity k = r.create();
    r.add<Transform>(k).position = {0.2f, 0.0f, 0.0f};
    r.add<Collider>(k, Collider::sphere(0.25f));
    r.add<RigidBody>(k, RigidBody::kinematic());
    // A kinematic body overlapping a *solid* static body is not reported or resolved.
    const Entity wall = add_static(r, Collider::box({0.5f, 0.5f, 0.5f}), {0.2f, 5.0f, 0.0f});
    r.get<Transform>(k).position.y = 5.0f;  // now inside the wall, outside the trigger

    world.step(r, kDt);
    EXPECT_TRUE(world.contacts().empty());
    EXPECT_EQ(pos(r, wall), glm::vec3(0.2f, 5.0f, 0.0f));

    r.get<Transform>(k).position = {0.2f, 0.0f, 0.0f};  // back inside the trigger
    world.step(r, kDt);
    ASSERT_EQ(world.contacts().size(), 1u);
    EXPECT_TRUE(world.contacts()[0].trigger);
    const bool has_both = (world.contacts()[0].a == z && world.contacts()[0].b == k) ||
                          (world.contacts()[0].a == k && world.contacts()[0].b == z);
    EXPECT_TRUE(has_both);
}

TEST(World, DynamicBodyOverlappingDynamicTriggerIsReportedNotPushed) {
    Registry r;
    PhysicsWorld world(no_gravity());
    Collider zone = Collider::sphere(1.0f);
    zone.is_trigger = true;
    const Entity z = add_dynamic(r, zone, {0.0f, 0.0f, 0.0f}, 1.0f);
    const Entity b = add_dynamic(r, Collider::sphere(0.5f), {0.5f, 0.0f, 0.0f}, 1.0f);
    world.step(r, kDt);
    EXPECT_EQ(world.contacts().size(), 1u);
    EXPECT_EQ(vel(r, z), glm::vec3(0.0f));
    EXPECT_EQ(vel(r, b), glm::vec3(0.0f));
    EXPECT_EQ(pos(r, b), glm::vec3(0.5f, 0.0f, 0.0f));
}

namespace {

struct Scene {
    Registry r;
    std::vector<Entity> bodies;
};

void build_scene(Scene& s) {
    add_static(s.r, Collider::box({20.0f, 0.5f, 20.0f}, 0.6f), {0.0f, -0.5f, 0.0f});
    for (int i = 0; i < 4; ++i) {
        s.bodies.push_back(add_dynamic(s.r, Collider::box({0.5f, 0.5f, 0.5f}, 0.5f, 0.1f),
                                       {0.1f * static_cast<float>(i), 0.5f + 1.1f * static_cast<float>(i),
                                        0.05f * static_cast<float>(i)},
                                       1.0f + static_cast<float>(i)));
    }
    for (int i = 0; i < 6; ++i) {
        const float f = static_cast<float>(i);
        s.bodies.push_back(add_dynamic(s.r, Collider::sphere(0.4f, 0.4f, 0.5f),
                                       {-3.0f + 0.7f * f, 3.0f + 0.9f * f, 0.3f * f}, 0.5f + 0.25f * f,
                                       {1.0f - 0.3f * f, 0.0f, 0.2f * f}));
    }
}

}  // namespace

TEST(World, ThousandStepsAreBitwiseDeterministic) {
    Scene a, b;
    build_scene(a);
    build_scene(b);
    PhysicsWorld wa, wb;
    for (int i = 0; i < 1000; ++i) {
        wa.step(a.r, kDt);
        wb.step(b.r, kDt);
    }
    ASSERT_EQ(a.bodies.size(), b.bodies.size());
    for (std::size_t i = 0; i < a.bodies.size(); ++i) {
        // Exact (bit-for-bit) comparison on purpose.
        EXPECT_EQ(pos(a.r, a.bodies[i]), pos(b.r, b.bodies[i])) << "body " << i;
        EXPECT_EQ(vel(a.r, a.bodies[i]), vel(b.r, b.bodies[i])) << "body " << i;
    }
    // Sanity: the scene is not trivially static.
    EXPECT_NE(pos(a.r, a.bodies[4]), glm::vec3(-3.0f, 3.0f, 0.0f));
}

TEST(World, SmallBoxStackIsStable) {
    Registry r;
    PhysicsWorld world;
    add_static(r, Collider::box({10.0f, 0.5f, 10.0f}, 0.6f), {0.0f, -0.5f, 0.0f});
    std::vector<Entity> boxes;
    for (int i = 0; i < 3; ++i) {
        boxes.push_back(add_dynamic(r, Collider::box({0.5f, 0.5f, 0.5f}, 0.6f),
                                    {0.0f, 0.5f + static_cast<float>(i), 0.0f}, 1.0f));
    }
    std::vector<float> y_mid;
    for (int i = 0; i < 600; ++i) {
        world.step(r, kDt);
        if (i == 299) {
            for (const Entity b : boxes) {
                y_mid.push_back(pos(r, b).y);
            }
        }
    }
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        // Residual vy is the unconverged part of one step of gravity (no warm starting, 10
        // iterations): measured <= 0.0098 m/s = 6% of g*dt. 0.02 allows 12%, still far from
        // sinking (a real failure gives ~g*dt = 0.16).
        EXPECT_NEAR(vel(r, boxes[i]).y, 0.0f, 0.02f) << "box " << i;
        // Boxes are exactly aligned and there is no tangential velocity: x/z must not move.
        EXPECT_NEAR(pos(r, boxes[i]).x, 0.0f, 1e-4f) << "box " << i;
        EXPECT_NEAR(pos(r, boxes[i]).z, 0.0f, 1e-4f) << "box " << i;
        // Settled: no drift between 5 s and 10 s (1 mm; a sinking stack would move cm).
        EXPECT_NEAR(pos(r, boxes[i]).y, y_mid[i], 1e-3f) << "box " << i;
    }
    // Bottom box sinks <= 0.05 m (measured 0.029) and neighbours overlap <= 0.06 m (measured
    // 0.039), i.e. ~1.5x margin over the measured steady state; the order stays vertical.
    EXPECT_GT(pos(r, boxes[0]).y, 0.45f);
    EXPECT_GT(pos(r, boxes[1]).y - pos(r, boxes[0]).y, 0.94f);
    EXPECT_GT(pos(r, boxes[2]).y - pos(r, boxes[1]).y, 0.94f);
    EXPECT_LT(pos(r, boxes[1]).y - pos(r, boxes[0]).y, 1.0f);
    EXPECT_LT(pos(r, boxes[2]).y - pos(r, boxes[1]).y, 1.0f);
}
