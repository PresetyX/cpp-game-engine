#include <gtest/gtest.h>

#include <vector>

#include "engine/physics/solver.hpp"

using engine::physics::ContactConstraint;
using engine::physics::SolverBody;
using engine::physics::SolverSettings;
using engine::physics::mix_friction;
using engine::physics::mix_restitution;
using engine::physics::solve_contacts;

namespace {

struct Dyn {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    float inverse_mass = 1.0f;

    SolverBody body() { return SolverBody{&position, &velocity, inverse_mass}; }
};

ContactConstraint make(SolverBody a, SolverBody b, const glm::vec3& n, float depth, float mu,
                       float e) {
    ContactConstraint c;
    c.a = a;
    c.b = b;
    c.normal = n;
    c.depth = depth;
    c.friction = mu;
    c.restitution = e;
    return c;
}

constexpr glm::vec3 kUp{0.0f, 1.0f, 0.0f};

}  // namespace

TEST(Solver, MixingRules) {
    EXPECT_FLOAT_EQ(mix_friction(0.5f, 0.5f), 0.5f);
    EXPECT_FLOAT_EQ(mix_friction(0.0f, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(mix_restitution(0.2f, 0.9f), 0.9f);
}

TEST(Solver, ElasticHeadOnEqualMassesSwapsVelocities) {
    Dyn a, b;
    a.velocity = {2.0f, 0.0f, 0.0f};
    std::vector<ContactConstraint> cs{
        make(a.body(), b.body(), {1.0f, 0.0f, 0.0f}, 0.005f, 0.0f, 1.0f)};
    solve_contacts(cs, SolverSettings{});

    // Tolerance 1e-6: with equal masses the first pass is exact in float (2 - 2, 0 + 2), and
    // later passes add impulses of exactly 0; 1e-6 is ~8 ulp at magnitude 2 as a safety margin.
    EXPECT_NEAR(a.velocity.x, 0.0f, 1e-6f);
    EXPECT_NEAR(b.velocity.x, 2.0f, 1e-6f);
}

TEST(Solver, InelasticWhenRestitutionIsZero) {
    Dyn a, b;
    a.velocity = {2.0f, 0.0f, 0.0f};
    std::vector<ContactConstraint> cs{
        make(a.body(), b.body(), {1.0f, 0.0f, 0.0f}, 0.005f, 0.0f, 0.0f)};
    solve_contacts(cs, SolverSettings{});

    // Both bodies end with the common velocity 1 (same argument and tolerance as above).
    EXPECT_NEAR(a.velocity.x, 1.0f, 1e-6f);
    EXPECT_NEAR(b.velocity.x, 1.0f, 1e-6f);
}

TEST(Solver, ElasticUnequalMassesMatchClosedForm) {
    Dyn a, b;
    a.inverse_mass = 1.0f / 3.0f;  // m = 3
    b.inverse_mass = 1.0f;         // m = 1
    a.velocity = {4.0f, 0.0f, 0.0f};
    std::vector<ContactConstraint> cs{
        make(a.body(), b.body(), {1.0f, 0.0f, 0.0f}, 0.005f, 0.0f, 1.0f)};
    solve_contacts(cs, SolverSettings{});

    // 1D elastic: va' = (m1-m2)/(m1+m2)*va = 2, vb' = 2*m1/(m1+m2)*va = 6.
    // Tolerance 1e-5: inverse_mass 1/3 is rounded (rel. error 6e-8) and a few operations follow;
    // the expected error is ~1e-6 at magnitude 6, so 1e-5 leaves a 10x margin.
    EXPECT_NEAR(a.velocity.x, 2.0f, 1e-5f);
    EXPECT_NEAR(b.velocity.x, 6.0f, 1e-5f);
}

TEST(Solver, ChainConservesMomentum) {
    // A -> B -> C along x, three different masses, partially elastic.
    Dyn a, b, c;
    a.inverse_mass = 1.0f / 2.0f;
    b.inverse_mass = 1.0f / 4.0f;
    c.inverse_mass = 1.0f;
    a.velocity = {5.0f, 0.0f, 0.0f};
    b.velocity = {1.0f, 0.0f, 0.0f};
    c.velocity = {-2.0f, 0.0f, 0.0f};
    const auto momentum = [&] {
        return a.velocity.x / a.inverse_mass + b.velocity.x / b.inverse_mass +
               c.velocity.x / c.inverse_mass;
    };
    const float before = momentum();

    std::vector<ContactConstraint> cs{
        make(a.body(), b.body(), {1.0f, 0.0f, 0.0f}, 0.02f, 0.3f, 0.5f),
        make(b.body(), c.body(), {1.0f, 0.0f, 0.0f}, 0.02f, 0.3f, 0.5f)};
    solve_contacts(cs, SolverSettings{});

    // Each impulse is applied with equal and opposite sign, so only rounding breaks conservation.
    // Tolerance 1e-4: the total has magnitude ~10 (ulp ~1e-6) and accumulates ~40 impulse
    // applications (10 iterations x 2 constraints x 2 bodies); 1e-4 is ~100 ulp of margin.
    EXPECT_NEAR(momentum(), before, 1e-4f);
    // Sanity: the collision actually happened.
    EXPECT_LT(a.velocity.x, 5.0f);
}

TEST(Solver, BallAtRestOnStaticGroundStaysAtRest) {
    constexpr float kDt = 1.0f / 60.0f;
    Dyn ball;
    ball.position = {0.0f, 0.5f, 0.0f};
    ball.velocity = {0.0f, -9.81f * kDt, 0.0f};  // one step of gravity, as after integration
    const float y0 = ball.position.y;
    SolverBody ground;  // static
    std::vector<ContactConstraint> cs{make(ground, ball.body(), kUp, 0.005f, 0.5f, 0.0f)};
    solve_contacts(cs, SolverSettings{});

    // Tolerance 1e-6 m/s: the first pass cancels vy exactly (impulse = -vn / k with k = 1),
    // later passes add 0; the margin only covers one rounding of the sum.
    EXPECT_NEAR(ball.velocity.y, 0.0f, 1e-6f);
    // Depth 0.005 is below the 0.01 slop, so there must be no position correction at all.
    EXPECT_EQ(ball.position.y, y0);
}

TEST(Solver, SlowImpactBelowThresholdDoesNotBounce) {
    Dyn ball;
    ball.velocity = {0.0f, -0.5f, 0.0f};  // below the 1 m/s restitution threshold
    SolverBody ground;
    std::vector<ContactConstraint> cs{make(ground, ball.body(), kUp, 0.005f, 0.5f, 1.0f)};
    solve_contacts(cs, SolverSettings{});
    EXPECT_NEAR(ball.velocity.y, 0.0f, 1e-6f);  // same argument as the resting test
}

TEST(Solver, FastImpactBouncesWithRestitution) {
    Dyn ball;
    ball.velocity = {0.0f, -4.0f, 0.0f};
    SolverBody ground;
    std::vector<ContactConstraint> cs{make(ground, ball.body(), kUp, 0.005f, 0.5f, 0.5f)};
    solve_contacts(cs, SolverSettings{});
    // vy' = -e * vn0 = 2. Exact in float (powers of two); 1e-6 is a margin of a few ulp.
    EXPECT_NEAR(ball.velocity.y, 2.0f, 1e-6f);
}

TEST(Solver, CoulombFrictionRemovesMuTimesNormalImpulse) {
    constexpr float kDt = 1.0f / 60.0f;
    constexpr float kG = 9.81f;
    constexpr float kMu = 0.5f;
    Dyn box;
    box.velocity = {5.0f, -kG * kDt, 0.0f};
    SolverBody ground;
    std::vector<ContactConstraint> cs{make(ground, box.body(), kUp, 0.005f, kMu, 0.0f)};
    solve_contacts(cs, SolverSettings{});

    // The normal impulse is g*dt (unit mass), so friction removes mu*g*dt of tangential speed.
    // Tolerance 1e-5: values are O(5) with ulp ~5e-7, and ~4 operations separate the expected
    // value from the result, so 1e-5 gives a 5x margin.
    EXPECT_NEAR(box.velocity.x, 5.0f - kMu * kG * kDt, 1e-5f);
    EXPECT_NEAR(box.velocity.y, 0.0f, 1e-6f);
}

TEST(Solver, SlowSlidingBodyStopsExactlyWithinFrictionCone) {
    constexpr float kDt = 1.0f / 60.0f;
    Dyn box;
    box.velocity = {0.05f, -9.81f * kDt, 0.0f};  // 0.05 < mu*g*dt = 0.0818
    SolverBody ground;
    std::vector<ContactConstraint> cs{make(ground, box.body(), kUp, 0.005f, 0.5f, 0.0f)};
    solve_contacts(cs, SolverSettings{});
    // Static friction holds: the tangential speed is cancelled, up to one rounding (1e-6).
    EXPECT_NEAR(box.velocity.x, 0.0f, 1e-6f);
}

TEST(Solver, PositionCorrectionMovesOnlyDynamicBodyAndKeepsVelocity) {
    Dyn ball;
    ball.position = {0.0f, 0.4f, 0.0f};
    SolverBody ground;
    ground.position = nullptr;  // static
    std::vector<ContactConstraint> cs{make(ground, ball.body(), kUp, 0.11f, 0.5f, 0.0f)};
    solve_contacts(cs, SolverSettings{});

    // (0.11 - 0.01 slop) * 0.4 = 0.04. Tolerance 1e-6: three float operations on O(0.1) values.
    EXPECT_NEAR(ball.position.y, 0.44f, 1e-6f);
    EXPECT_EQ(ball.velocity.y, 0.0f);
}

TEST(Solver, KinematicBodyIsNeverWritten) {
    Dyn kin;
    kin.inverse_mass = 0.0f;
    kin.velocity = {-3.0f, 0.0f, 0.0f};  // moving toward the ball
    kin.position = {1.0f, 2.0f, 3.0f};
    Dyn ball;
    ball.position = {0.9f, 2.0f, 3.0f};
    std::vector<ContactConstraint> cs{
        make(kin.body(), ball.body(), {-1.0f, 0.0f, 0.0f}, 0.2f, 0.5f, 0.0f)};
    solve_contacts(cs, SolverSettings{});

    EXPECT_EQ(kin.velocity, glm::vec3(-3.0f, 0.0f, 0.0f));
    EXPECT_EQ(kin.position, glm::vec3(1.0f, 2.0f, 3.0f));
    // The ball is pushed to the kinematic body's velocity (restitution 0). 1e-5: a few
    // operations at magnitude 3 (ulp ~2.4e-7).
    EXPECT_NEAR(ball.velocity.x, -3.0f, 1e-5f);
}

TEST(Solver, IsDeterministicForTheSameInput) {
    const auto run = [] {
        Dyn a, b, c;
        a.velocity = {3.0f, 0.1f, 0.0f};
        b.velocity = {-1.0f, 0.0f, 0.2f};
        c.velocity = {0.0f, -2.0f, 0.0f};
        std::vector<ContactConstraint> cs{
            make(a.body(), b.body(), {1.0f, 0.0f, 0.0f}, 0.03f, 0.4f, 0.3f),
            make(b.body(), c.body(), kUp, 0.02f, 0.4f, 0.3f)};
        solve_contacts(cs, SolverSettings{});
        return std::vector<glm::vec3>{a.velocity, b.velocity, c.velocity, a.position, b.position,
                                      c.position};
    };
    EXPECT_EQ(run(), run());  // exact (bit-for-bit) comparison on purpose
}
