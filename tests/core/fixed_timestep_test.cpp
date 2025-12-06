#include <gtest/gtest.h>

#include <vector>

#include "engine/core/fixed_timestep.hpp"

using engine::FixedTimestep;

namespace {
// Powers of two are exact in binary floating point, so comparisons are exact.
constexpr double kStep = 1.0 / 64.0;
}  // namespace

TEST(FixedTimestep, ExactlyOneStep) {
    FixedTimestep ts(kStep);
    EXPECT_EQ(ts.advance(kStep), 1);
    EXPECT_DOUBLE_EQ(ts.alpha(), 0.0);
}

TEST(FixedTimestep, SmallerThanStepRunsNothingAndSetsAlpha) {
    FixedTimestep ts(kStep);
    EXPECT_EQ(ts.advance(kStep / 4), 0);
    EXPECT_DOUBLE_EQ(ts.alpha(), 0.25);
}

TEST(FixedTimestep, RemainderCarriesAcrossFrames) {
    FixedTimestep ts(kStep);
    EXPECT_EQ(ts.advance(kStep * 0.75), 0);
    EXPECT_EQ(ts.advance(kStep * 0.75), 1);  // 1.5 steps accumulated
    EXPECT_DOUBLE_EQ(ts.alpha(), 0.5);
}

TEST(FixedTimestep, LargeFrameRunsSeveralSteps) {
    FixedTimestep ts(kStep);
    EXPECT_EQ(ts.advance(kStep * 5.5), 5);
    EXPECT_DOUBLE_EQ(ts.alpha(), 0.5);
}

TEST(FixedTimestep, FrameTimeIsClamped) {
    FixedTimestep ts(kStep, /*max_frame_time=*/0.25);
    // 0.25 s / (1/64 s) = 16 steps, regardless of the huge stall.
    EXPECT_EQ(ts.advance(100.0), 16);
}

TEST(FixedTimestep, NegativeFrameTimeIsIgnored) {
    FixedTimestep ts(kStep);
    EXPECT_EQ(ts.advance(-1.0), 0);
    EXPECT_DOUBLE_EQ(ts.alpha(), 0.0);
}

TEST(FixedTimestep, AlphaStaysInUnitInterval) {
    FixedTimestep ts(kStep);
    const double frames[] = {0.003, 0.021, 0.0167, 0.05, 0.0001, 0.2};
    for (double dt : frames) {
        ts.advance(dt);
        EXPECT_GE(ts.alpha(), 0.0);
        EXPECT_LT(ts.alpha(), 1.0);
    }
}

TEST(FixedTimestep, DeterministicForSameInput) {
    const double frames[] = {0.016, 0.017, 0.033, 0.001, 0.016, 0.05, 0.012};
    FixedTimestep a(kStep);
    FixedTimestep b(kStep);
    for (double dt : frames) {
        EXPECT_EQ(a.advance(dt), b.advance(dt));
        EXPECT_DOUBLE_EQ(a.alpha(), b.alpha());
    }
}

TEST(FixedTimestep, TotalStepsMatchTotalTime) {
    // 64 frames of 1/64 s plus tiny jitter-free subdivisions: 1 second = 64 steps.
    FixedTimestep ts(kStep);
    int total = 0;
    for (int i = 0; i < 256; ++i) {
        total += ts.advance(kStep / 4);
    }
    EXPECT_EQ(total, 64);
}

TEST(FixedTimestep, UpdateCallsCallbackWithFixedStep) {
    FixedTimestep ts(kStep);
    std::vector<double> seen;
    const double alpha = ts.update(kStep * 3.25, [&](double dt) { seen.push_back(dt); });
    ASSERT_EQ(seen.size(), 3u);
    for (double dt : seen) {
        EXPECT_DOUBLE_EQ(dt, kStep);
    }
    EXPECT_DOUBLE_EQ(alpha, 0.25);
}

TEST(FixedTimestep, ResetClearsAccumulator) {
    FixedTimestep ts(kStep);
    ts.advance(kStep * 0.5);
    ts.reset();
    EXPECT_DOUBLE_EQ(ts.alpha(), 0.0);
}
