#include <gtest/gtest.h>

#include <algorithm>
#include <type_traits>
#include <vector>

#include "engine/ecs/registry.hpp"

using engine::Entity;
using engine::Registry;

namespace {
struct A {
    int v;
};
struct B {
    int v;
};
struct C {
    int v;
};
struct Position {
    float x, y, z;
};
struct Velocity {
    float x, y, z;
};
struct Tag {
    int v;
};

bool by_index(Entity a, Entity b) {
    return a.index < b.index;
}

std::vector<Entity> sorted(std::vector<Entity> v) {
    std::sort(v.begin(), v.end(), by_index);
    return v;
}

template <typename ViewT>
std::vector<Entity> collect(const ViewT& view) {
    std::vector<Entity> out;
    for (const Entity e : view) {
        out.push_back(e);
    }
    return sorted(std::move(out));
}
}  // namespace

TEST(View, SingleComponentVisitsAllOwners) {
    Registry r;
    std::vector<Entity> with_a;
    for (int i = 0; i < 6; ++i) {
        const Entity e = r.create();
        if (i % 2 == 0) {
            r.add<A>(e, i);
            with_a.push_back(e);
        }
    }
    EXPECT_EQ(collect(r.view<A>()), sorted(with_a));
}

TEST(View, TwoAndThreeComponentsVisitOnlyIntersection) {
    Registry r;
    std::vector<Entity> ab;
    std::vector<Entity> abc;
    for (int i = 0; i < 12; ++i) {
        const Entity e = r.create();
        if (i % 2 == 0) {
            r.add<A>(e, i);
        }
        if (i % 3 == 0) {
            r.add<B>(e, i);
        }
        if (i % 4 == 0) {
            r.add<C>(e, i);
        }
        if (i % 2 == 0 && i % 3 == 0) {
            ab.push_back(e);
            if (i % 4 == 0) {
                abc.push_back(e);
            }
        }
    }
    EXPECT_EQ(collect(r.view<A, B>()), sorted(ab));
    EXPECT_EQ(collect(r.view<A, B, C>()), sorted(abc));
}

TEST(View, IgnoresEntitiesMissingAnyComponent) {
    Registry r;
    const Entity both = r.create();
    const Entity only_a = r.create();
    const Entity only_b = r.create();
    r.add<A>(both, 1);
    r.add<B>(both, 1);
    r.add<A>(only_a, 1);
    r.add<B>(only_b, 1);
    EXPECT_EQ(collect(r.view<A, B>()), std::vector<Entity>{both});
}

TEST(View, EmptyWhenComponentTypeWasNeverAdded) {
    Registry r;
    const Entity e = r.create();
    r.add<A>(e, 1);
    int visits = 0;
    r.view<A, B>().each([&](A&, B&) { ++visits; });
    for (const Entity x : r.view<A, B>()) {
        (void)x;
        ++visits;
    }
    const Registry& cr = r;
    for (const Entity x : cr.view<A, B>()) {
        (void)x;
        ++visits;
    }
    EXPECT_EQ(visits, 0);
    EXPECT_EQ((r.view<A, B>().size_hint()), 0u);
}

TEST(View, EmptyRegistry) {
    Registry r;
    auto view = r.view<A>();
    EXPECT_TRUE(view.begin() == view.end());
}

TEST(View, SmallestPoolIsPivot) {
    Registry r;
    for (int i = 0; i < 10; ++i) {
        const Entity e = r.create();
        r.add<A>(e, i);
        if (i < 3) {
            r.add<B>(e, i);
        }
    }
    EXPECT_EQ((r.view<A, B>().size_hint()), 3u);
    EXPECT_EQ((r.view<B, A>().size_hint()), 3u);
}

TEST(View, EachWithAndWithoutEntityParameter) {
    Registry r;
    const Entity e = r.create();
    r.add<A>(e, 4);
    r.add<B>(e, 5);
    int sum_plain = 0;
    r.view<A, B>().each([&](A& a, B& b) { sum_plain += a.v + b.v; });
    Entity seen = engine::null_entity;
    r.view<A, B>().each([&](Entity x, A&, B&) { seen = x; });
    EXPECT_EQ(sum_plain, 9);
    EXPECT_EQ(seen, e);
}

TEST(View, RangeForMatchesEach) {
    Registry r;
    for (int i = 0; i < 8; ++i) {
        const Entity e = r.create();
        r.add<A>(e, i);
        if (i % 2 == 0) {
            r.add<B>(e, i);
        }
    }
    std::vector<Entity> via_each;
    r.view<A, B>().each([&](Entity e, A&, B&) { via_each.push_back(e); });
    EXPECT_EQ(collect(r.view<A, B>()), sorted(via_each));
}

TEST(View, ModificationsPersist) {
    Registry r;
    const Entity e = r.create();
    r.add<Position>(e, 1.0f, 2.0f, 3.0f);
    r.add<Velocity>(e, 0.5f, 0.5f, 0.5f);
    r.view<Position, const Velocity>().each([](Position& p, const Velocity& v) {
        p.x += v.x;
        p.y += v.y;
        p.z += v.z;
    });
    EXPECT_EQ(r.get<Position>(e).x, 1.5f);
    EXPECT_EQ(r.get<Position>(e).z, 3.5f);
}

TEST(View, ConstViewYieldsConstReferences) {
    Registry r;
    const Entity e = r.create();
    r.add<Velocity>(e, 1.0f, 1.0f, 1.0f);
    auto view = r.view<const Velocity>();
    static_assert(std::is_same_v<decltype(view.get<const Velocity>(e)), const Velocity&>);
    const Registry& cr = r;
    auto cview = cr.view<Velocity>();
    static_assert(std::is_same_v<decltype(cview.get<const Velocity>(e)), const Velocity&>);
    int visits = 0;
    cview.each([&](const Velocity&) { ++visits; });
    EXPECT_EQ(visits, 1);
}

TEST(View, IterationAfterRemovals) {
    Registry r;
    std::vector<Entity> es;
    for (int i = 0; i < 10; ++i) {
        es.push_back(r.create());
        r.add<A>(es.back(), i);
        r.add<B>(es.back(), i);
    }
    r.remove<B>(es[2]);
    r.destroy(es[5]);
    std::vector<Entity> expected;
    for (std::size_t i = 0; i < es.size(); ++i) {
        if (i != 2 && i != 5) {
            expected.push_back(es[i]);
        }
    }
    EXPECT_EQ(collect(r.view<A, B>()), sorted(expected));
}

TEST(View, DestroyCurrentEntityDuringEach) {
    Registry r;
    std::vector<Entity> created;
    for (int i = 0; i < 10; ++i) {
        const Entity e = r.create();
        r.add<A>(e, i);
        r.add<B>(e, i);
        created.push_back(e);
    }
    std::vector<Entity> visited;
    r.view<A, B>().each([&](Entity e, A&, B&) {
        visited.push_back(e);
        r.destroy(e);
    });
    EXPECT_EQ(sorted(visited), sorted(created));  // each exactly once
    EXPECT_EQ(r.alive_count(), 0u);
}

TEST(View, RemovePivotComponentFromCurrentDuringRangeFor) {
    Registry r;
    std::vector<Entity> created;
    for (int i = 0; i < 10; ++i) {
        const Entity e = r.create();
        r.add<A>(e, i);
        r.add<B>(e, i);
        created.push_back(e);
    }
    std::vector<Entity> visited;
    for (const Entity e : r.view<A, B>()) {
        visited.push_back(e);
        r.remove<A>(e);
    }
    EXPECT_EQ(sorted(visited), sorted(created));
    EXPECT_EQ(r.view<A>().size_hint(), 0u);
}

TEST(View, EntitiesCreatedDuringIterationAreNotVisited) {
    Registry r;
    for (int i = 0; i < 5; ++i) {
        const Entity e = r.create();
        r.add<A>(e, i);
    }
    int visits = 0;
    r.view<A>().each([&](A&) {
        ++visits;
        const Entity extra = r.create();
        r.add<B>(extra, 0);  // different component type than the view
    });
    EXPECT_EQ(visits, 5);
    EXPECT_EQ(r.view<B>().size_hint(), 5u);
}

TEST(View, DestroyingOtherEntitiesIsMemorySafe) {
    Registry r;
    std::vector<Entity> es;
    for (int i = 0; i < 20; ++i) {
        es.push_back(r.create());
        r.add<A>(es.back(), i);
    }
    // Result is unspecified (entities may be skipped); it must not crash and
    // the survivors must stay consistent.
    int visits = 0;
    r.view<A>().each([&](Entity, A&) {
        ++visits;
        r.destroy(es.front());
    });
    EXPECT_GE(visits, 1);
    EXPECT_LE(visits, 20);
    r.view<A>().each([&](Entity e, A&) { EXPECT_TRUE(r.alive(e)); });
}

TEST(View, HundredThousandEntities) {
    constexpr int kCount = 100000;
    Registry r;
    for (int i = 0; i < kCount; ++i) {
        const Entity e = r.create();
        r.add<Position>(e, static_cast<float>(i), 0.0f, 0.0f);
        if (i % 10 == 0) {
            r.add<Tag>(e, i);
        }
    }
    int all = 0;
    r.view<Position>().each([&](Position&) { ++all; });
    int tagged = 0;
    r.view<Position, Tag>().each([&](const Position& p, const Tag& t) {
        EXPECT_EQ(static_cast<int>(p.x), t.v);
        ++tagged;
    });
    EXPECT_EQ(all, kCount);
    EXPECT_EQ(tagged, kCount / 10);
}
