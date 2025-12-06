#include <gtest/gtest.h>

#include "engine/ecs/entity_pool.hpp"

using engine::Entity;
using engine::EntityPool;

TEST(EntityPool, CreatesSequentialIndicesWithGenerationZero) {
    EntityPool pool;
    for (std::uint32_t i = 0; i < 3; ++i) {
        const Entity e = pool.create();
        EXPECT_EQ(e.index, i);
        EXPECT_EQ(e.generation, 0u);
        EXPECT_TRUE(pool.alive(e));
    }
    EXPECT_EQ(pool.alive_count(), 3u);
}

TEST(EntityPool, DestroyMakesEntityNotAlive) {
    EntityPool pool;
    const Entity e = pool.create();
    EXPECT_TRUE(pool.destroy(e));
    EXPECT_FALSE(pool.alive(e));
    EXPECT_EQ(pool.alive_count(), 0u);
}

TEST(EntityPool, DestroyTwiceIsNoOp) {
    EntityPool pool;
    const Entity a = pool.create();
    const Entity b = pool.create();
    EXPECT_TRUE(pool.destroy(a));
    EXPECT_FALSE(pool.destroy(a));
    EXPECT_EQ(pool.alive_count(), 1u);
    EXPECT_TRUE(pool.alive(b));
}

TEST(EntityPool, ReusesIndexWithIncrementedGeneration) {
    EntityPool pool;
    const Entity old_e = pool.create();
    pool.destroy(old_e);
    const Entity new_e = pool.create();
    EXPECT_EQ(new_e.index, old_e.index);
    EXPECT_EQ(new_e.generation, old_e.generation + 1);
    EXPECT_FALSE(pool.alive(old_e));
    EXPECT_TRUE(pool.alive(new_e));
}

TEST(EntityPool, DoubleDestroyDoesNotDuplicateFreeIndex) {
    EntityPool pool;
    const Entity e = pool.create();
    pool.destroy(e);
    pool.destroy(e);
    const Entity a = pool.create();
    const Entity b = pool.create();
    EXPECT_NE(a.index, b.index);
}

TEST(EntityPool, FreeListIsLifo) {
    EntityPool pool;
    const Entity a = pool.create();
    const Entity b = pool.create();
    pool.destroy(a);
    pool.destroy(b);
    EXPECT_EQ(pool.create().index, b.index);
    EXPECT_EQ(pool.create().index, a.index);
}

TEST(EntityPool, FutureGenerationOfFreeSlotIsNotAlive) {
    EntityPool pool;
    const Entity e = pool.create();
    pool.destroy(e);
    // The slot is free and its stored generation is e.generation + 1.
    EXPECT_FALSE(pool.alive(Entity{e.index, e.generation + 1}));
}

TEST(EntityPool, NullAndUnknownEntitiesAreNeverAlive) {
    EntityPool pool;
    EXPECT_FALSE(pool.alive(engine::null_entity));
    EXPECT_FALSE(pool.alive(Entity{999, 0}));
    EXPECT_FALSE(pool.destroy(engine::null_entity));
    EXPECT_FALSE(pool.destroy(Entity{999, 0}));
}

TEST(EntityPool, ReusesIndicesBeforeGrowing) {
    EntityPool pool;
    for (int i = 0; i < 100; ++i) {
        pool.destroy(pool.create());
    }
    EXPECT_EQ(pool.slot_count(), 1u);
}
