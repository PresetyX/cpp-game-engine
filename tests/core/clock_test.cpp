#include <gtest/gtest.h>

#include "engine/core/clock.hpp"

TEST(Clock, NowIsMonotonic) {
    const double a = engine::Clock::now_seconds();
    const double b = engine::Clock::now_seconds();
    EXPECT_GE(b, a);
}

TEST(FrameTimer, FirstTickIsZero) {
    engine::FrameTimer timer;
    EXPECT_EQ(timer.tick(), 0.0);
}

TEST(FrameTimer, LaterTicksAreNonNegative) {
    engine::FrameTimer timer;
    timer.tick();
    EXPECT_GE(timer.tick(), 0.0);
    EXPECT_GE(timer.tick(), 0.0);
}
