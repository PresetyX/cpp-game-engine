#include <gtest/gtest.h>

#include <algorithm>

#include "engine/core/version.hpp"

TEST(Version, IsNotEmptyAndHasThreeComponents) {
    const auto v = engine::version();
    ASSERT_FALSE(v.empty());
    EXPECT_EQ(std::count(v.begin(), v.end(), '.'), 2);
}
