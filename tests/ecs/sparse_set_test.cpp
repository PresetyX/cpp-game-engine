#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "engine/ecs/sparse_set.hpp"

using engine::Entity;
using engine::SparseSet;

namespace {
struct Value {
    int v;
};
struct Owned {
    std::unique_ptr<int> p;
};
}  // namespace

TEST(SparseSet, EmplaceContainsGet) {
    SparseSet<Value> set;
    const Entity e{3, 0};
    set.emplace(e, 42);
    EXPECT_TRUE(set.contains(e));
    EXPECT_EQ(set.get(e).v, 42);
    EXPECT_EQ(set.size(), 1u);
}

TEST(SparseSet, EmplaceExistingReplacesValue) {
    SparseSet<Value> set;
    const Entity e{1, 0};
    set.emplace(e, 1);
    set.emplace(e, 2);
    EXPECT_EQ(set.get(e).v, 2);
    EXPECT_EQ(set.size(), 1u);
}

TEST(SparseSet, DifferentGenerationDoesNotMatch) {
    SparseSet<Value> set;
    set.emplace(Entity{3, 0}, 1);
    EXPECT_FALSE(set.contains(Entity{3, 1}));
    EXPECT_EQ(set.try_get(Entity{3, 1}), nullptr);
}

TEST(SparseSet, OutOfRangeAndNullAreNotContained) {
    SparseSet<Value> set;
    set.emplace(Entity{0, 0}, 1);
    EXPECT_FALSE(set.contains(Entity{1000, 0}));
    EXPECT_FALSE(set.contains(engine::null_entity));
}

TEST(SparseSet, RemoveMissingReturnsFalse) {
    SparseSet<Value> set;
    EXPECT_FALSE(set.remove(Entity{0, 0}));
    set.emplace(Entity{1, 0}, 1);
    EXPECT_FALSE(set.remove(Entity{0, 0}));
    EXPECT_EQ(set.size(), 1u);
}

TEST(SparseSet, SwapAndPopKeepsEntitiesAlignedWithData) {
    SparseSet<Value> set;
    for (std::uint32_t i = 0; i < 5; ++i) {
        set.emplace(Entity{i, 0}, static_cast<int>(i));
    }
    EXPECT_TRUE(set.remove(Entity{1, 0}));
    ASSERT_EQ(set.size(), 4u);
    EXPECT_FALSE(set.contains(Entity{1, 0}));
    // The last element moved into the freed position.
    EXPECT_EQ(set.entities()[1], (Entity{4, 0}));
    for (std::size_t i = 0; i < set.size(); ++i) {
        EXPECT_EQ(set.data()[i].v, static_cast<int>(set.entities()[i].index));
    }
    for (std::uint32_t i : {0u, 2u, 3u, 4u}) {
        EXPECT_EQ(set.get(Entity{i, 0}).v, static_cast<int>(i));
    }
}

TEST(SparseSet, RemoveLastAndOnlyElement) {
    SparseSet<Value> set;
    set.emplace(Entity{0, 0}, 0);
    set.emplace(Entity{1, 0}, 1);
    EXPECT_TRUE(set.remove(Entity{1, 0}));
    EXPECT_EQ(set.get(Entity{0, 0}).v, 0);
    EXPECT_TRUE(set.remove(Entity{0, 0}));
    EXPECT_TRUE(set.empty());
}

TEST(SparseSet, MoveOnlyComponentSurvivesSwapAndPop) {
    SparseSet<Owned> set;
    set.emplace(Entity{0, 0}, std::make_unique<int>(10));
    set.emplace(Entity{1, 0}, std::make_unique<int>(11));
    set.emplace(Entity{2, 0}, std::make_unique<int>(12));
    set.remove(Entity{0, 0});
    EXPECT_EQ(*set.get(Entity{1, 0}).p, 11);
    EXPECT_EQ(*set.get(Entity{2, 0}).p, 12);
}

TEST(SparseSet, NonAggregateComponent) {
    SparseSet<std::string> set;
    set.emplace(Entity{0, 0}, 3u, 'a');
    EXPECT_EQ(set.get(Entity{0, 0}), "aaa");
}

TEST(SparseSet, ClearEmpties) {
    SparseSet<Value> set;
    set.emplace(Entity{0, 0}, 1);
    set.clear();
    EXPECT_TRUE(set.empty());
    EXPECT_FALSE(set.contains(Entity{0, 0}));
}
