#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include "engine/ecs/registry.hpp"

using engine::Entity;
using engine::Registry;

namespace {
struct Position {
    float x, y, z;
};
struct Velocity {
    float x, y, z;
};
struct Health {
    int hp;
};
struct Owned {
    std::unique_ptr<int> p;
};
}  // namespace

TEST(Registry, AddGetHasRemove) {
    Registry r;
    const Entity e = r.create();
    EXPECT_FALSE(r.has<Health>(e));
    r.add<Health>(e, 10);
    EXPECT_TRUE(r.has<Health>(e));
    EXPECT_EQ(r.get<Health>(e).hp, 10);
    r.get<Health>(e).hp = 7;
    EXPECT_EQ(r.get<Health>(e).hp, 7);
    EXPECT_TRUE(r.remove<Health>(e));
    EXPECT_FALSE(r.has<Health>(e));
    EXPECT_FALSE(r.remove<Health>(e));
}

TEST(Registry, AggregateComponentConstruction) {
    Registry r;
    const Entity e = r.create();
    r.add<Position>(e, 1.0f, 2.0f, 3.0f);
    EXPECT_EQ(r.get<Position>(e).y, 2.0f);
}

TEST(Registry, TryGetIsNullWithoutPoolEntityOrWhenDead) {
    Registry r;
    const Entity e = r.create();
    EXPECT_EQ(r.try_get<Health>(e), nullptr);  // pool never created
    r.add<Health>(e, 1);
    EXPECT_NE(r.try_get<Health>(e), nullptr);
    EXPECT_EQ(r.try_get<Health>(engine::null_entity), nullptr);
    r.destroy(e);
    EXPECT_EQ(r.try_get<Health>(e), nullptr);
}

TEST(Registry, AddReplacesExistingComponent) {
    Registry r;
    const Entity e = r.create();
    r.add<Health>(e, 1);
    r.add<Health>(e, 2);
    EXPECT_EQ(r.get<Health>(e).hp, 2);
}

TEST(Registry, AddToDeadOrNullEntityThrows) {
    Registry r;
    const Entity e = r.create();
    r.destroy(e);
    EXPECT_THROW(r.add<Health>(e, 1), std::invalid_argument);
    EXPECT_THROW(r.add<Health>(engine::null_entity, 1), std::invalid_argument);
    // No pool was created by the failed adds.
    EXPECT_EQ(r.view<Health>().size_hint(), 0u);
}

TEST(Registry, DestroyRemovesAllComponents) {
    Registry r;
    const Entity e = r.create();
    r.add<Position>(e, 0.0f, 0.0f, 0.0f);
    r.add<Health>(e, 5);
    EXPECT_TRUE(r.destroy(e));
    EXPECT_FALSE(r.destroy(e));
    EXPECT_EQ(r.view<Position>().size_hint(), 0u);
    EXPECT_EQ(r.view<Health>().size_hint(), 0u);
}

TEST(Registry, ReusedIndexDoesNotInheritComponents) {
    Registry r;
    const Entity old_e = r.create();
    r.add<Health>(old_e, 5);
    r.destroy(old_e);
    const Entity new_e = r.create();
    ASSERT_EQ(new_e.index, old_e.index);
    EXPECT_FALSE(r.has<Health>(new_e));
}

TEST(Registry, StaleHandleDoesNotSeeNewEntityComponents) {
    Registry r;
    const Entity old_e = r.create();
    r.destroy(old_e);
    const Entity new_e = r.create();
    ASSERT_EQ(new_e.index, old_e.index);
    r.add<Health>(new_e, 5);
    EXPECT_FALSE(r.has<Health>(old_e));
    EXPECT_EQ(r.try_get<Health>(old_e), nullptr);
    EXPECT_FALSE(r.remove<Health>(old_e));
    EXPECT_THROW(r.add<Health>(old_e, 1), std::invalid_argument);
    EXPECT_FALSE(r.destroy(old_e));
    EXPECT_TRUE(r.has<Health>(new_e));  // unaffected by stale calls
}

TEST(Registry, DestroyKeepsOtherEntitiesData) {
    Registry r;
    std::vector<Entity> es;
    for (int i = 0; i < 5; ++i) {
        es.push_back(r.create());
        r.add<Health>(es.back(), i);
    }
    r.destroy(es[1]);
    for (int i : {0, 2, 3, 4}) {
        EXPECT_EQ(r.get<Health>(es[static_cast<std::size_t>(i)]).hp, i);
    }
}

TEST(Registry, ConstAccess) {
    Registry r;
    const Entity e = r.create();
    r.add<Health>(e, 3);
    const Registry& cr = r;
    EXPECT_TRUE(cr.has<Health>(e));
    EXPECT_EQ(cr.get<Health>(e).hp, 3);
    EXPECT_NE(cr.try_get<Health>(e), nullptr);
    EXPECT_EQ(cr.try_get<Velocity>(e), nullptr);
}

TEST(Registry, MoveOnlyComponent) {
    Registry r;
    const Entity a = r.create();
    const Entity b = r.create();
    r.add<Owned>(a, std::make_unique<int>(7));
    r.add<Owned>(b, std::make_unique<int>(8));
    r.remove<Owned>(a);
    EXPECT_EQ(*r.get<Owned>(b).p, 8);
}

TEST(Registry, HundredThousandCreateDestroyReusesSlots) {
    constexpr std::size_t kCount = 100000;
    Registry r;
    std::vector<Entity> first;
    first.reserve(kCount);
    for (std::size_t i = 0; i < kCount; ++i) {
        first.push_back(r.create());
    }
    EXPECT_EQ(r.alive_count(), kCount);
    for (const Entity e : first) {
        EXPECT_TRUE(r.destroy(e));
    }
    EXPECT_EQ(r.alive_count(), 0u);
    for (std::size_t i = 0; i < kCount; ++i) {
        r.create();
    }
    EXPECT_EQ(r.slot_count(), kCount);
    for (const Entity e : first) {
        EXPECT_FALSE(r.alive(e));
    }
}
