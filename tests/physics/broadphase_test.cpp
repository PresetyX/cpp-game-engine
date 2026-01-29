#include "engine/physics/broadphase.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <tuple>

namespace engine::physics {
namespace {

using math::Aabb;

BroadphaseProxy P(std::uint32_t id, glm::vec3 mn, glm::vec3 mx, bool is_static = false) {
    return BroadphaseProxy{Entity{id, 0}, Aabb{mn, mx}, is_static};
}

BroadphaseProxy Box(std::uint32_t id, glm::vec3 c, float h = 0.5f) {
    return P(id, c - glm::vec3(h), c + glm::vec3(h));
}

bool same(const std::vector<BroadphasePair>& l, const std::vector<BroadphasePair>& r) {
    return l.size() == r.size() &&
           std::equal(l.begin(), l.end(), r.begin(),
                      [](const BroadphasePair& x, const BroadphasePair& y) {
                          return x.a == y.a && x.b == y.b;
                      });
}

std::vector<BroadphasePair> sap(const std::vector<BroadphaseProxy>& v,
                                BroadphaseOptions o = {}) {
    return sweep_and_prune(v, o);
}

TEST(Broadphase, EmptyAndSingle) {
    EXPECT_TRUE(sap({}).empty());
    EXPECT_TRUE(sap({Box(1, {0, 0, 0})}).empty());
}

TEST(Broadphase, TouchingFaceEdgeCornerCount) {
    EXPECT_EQ(sap({Box(1, {0, 0, 0}), Box(2, {1, 0, 0})}).size(), 1u);
    EXPECT_EQ(sap({Box(1, {0, 0, 0}), Box(2, {1, 1, 0})}).size(), 1u);
    EXPECT_EQ(sap({Box(1, {0, 0, 0}), Box(2, {1, 1, 1})}).size(), 1u);
    const float gap = std::nextafter(1.0f, 2.0f);
    EXPECT_TRUE(sap({Box(1, {0, 0, 0}), Box(2, {gap, 0, 0})}).empty());
}

TEST(Broadphase, IdenticalBoxesGiveAllPairs) {
    std::vector<BroadphaseProxy> v;
    for (std::uint32_t i = 0; i < 5; ++i) {
        v.push_back(Box(i, {0, 0, 0}));
    }
    EXPECT_EQ(sap(v).size(), 10u);
}

TEST(Broadphase, EqualMinXMatchesBruteForce) {
    std::vector<BroadphaseProxy> v = {P(1, {0, 0, 0}, {1, 1, 1}), P(2, {0, 0.5f, 0}, {5, 2, 1}),
                                      P(3, {0, 10, 0}, {2, 11, 1}), P(4, {0, 0, 3}, {3, 1, 4})};
    EXPECT_TRUE(same(sap(v), brute_force_pairs(v)));
}

TEST(Broadphase, NestedBox) {
    EXPECT_EQ(sap({P(1, {-5, -5, -5}, {5, 5, 5}), Box(2, {0, 0, 0}, 0.1f)}).size(), 1u);
}

TEST(Broadphase, DegeneratePoints) {
    EXPECT_EQ(sap({P(1, {1, 1, 1}, {1, 1, 1}), P(2, {1, 1, 1}, {1, 1, 1})}).size(), 1u);
    EXPECT_EQ(sap({Box(1, {0, 0, 0}), P(2, {0.5f, 0, 0}, {0.5f, 0, 0})}).size(), 1u);
    EXPECT_TRUE(sap({Box(1, {0, 0, 0}), P(2, {0.6f, 0, 0}, {0.6f, 0, 0})}).empty());
}

TEST(Broadphase, XOverlapOnlyIsNotAPair) {
    EXPECT_TRUE(sap({Box(1, {0, 0, 0}), Box(2, {0, 5, 0})}).empty());
    EXPECT_TRUE(sap({Box(1, {0, 0, 0}), Box(2, {0, 0, 5})}).empty());
}

TEST(Broadphase, CanonicalOrientation) {
    const auto r = sap({Box(5, {0, 0, 0}), Box(2, {0, 0, 0})});
    ASSERT_EQ(r.size(), 1u);
    EXPECT_EQ(r[0].a.index, 2u);
    EXPECT_EQ(r[0].b.index, 5u);
}

TEST(Broadphase, PermutationInvariant) {
    std::vector<BroadphaseProxy> v;
    for (std::uint32_t i = 0; i < 40; ++i) {
        v.push_back(Box(i, {float(i % 5), float(i % 3), 0}));
    }
    const auto base = sap(v);
    std::mt19937 rng(7);
    std::shuffle(v.begin(), v.end(), rng);
    EXPECT_TRUE(same(base, sap(v)));
}

TEST(Broadphase, StaticFilter) {
    std::vector<BroadphaseProxy> v = {Box(1, {0, 0, 0}), Box(2, {0, 0, 0}), Box(3, {0, 0, 0})};
    v[0].is_static = true;
    v[1].is_static = true;
    EXPECT_EQ(sap(v).size(), 3u);
    BroadphaseOptions o;
    o.skip_static_pairs = true;
    const auto r = sap(v, o);
    ASSERT_EQ(r.size(), 2u);
    EXPECT_EQ(r[0].a.index, 1u);
    EXPECT_EQ(r[0].b.index, 3u);
    EXPECT_EQ(r[1].a.index, 2u);
    EXPECT_EQ(r[1].b.index, 3u);
}

TEST(Broadphase, InfiniteBoxPairsWithAll) {
    const float inf = std::numeric_limits<float>::infinity();
    std::vector<BroadphaseProxy> v = {P(1, {-inf, -inf, -inf}, {inf, inf, inf}),
                                      Box(2, {100, 0, 0}), Box(3, {-100, 5, 5})};
    EXPECT_EQ(sap(v).size(), 2u);
    EXPECT_TRUE(same(sap(v), brute_force_pairs(v)));
}

#ifdef NDEBUG
TEST(Broadphase, NanProxyIgnored) {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    std::vector<BroadphaseProxy> v = {Box(1, {0, 0, 0}), Box(2, {0, 0, 0}),
                                      P(3, {nan, 0, 0}, {1, 1, 1})};
    EXPECT_EQ(sap(v).size(), 1u);
    EXPECT_TRUE(same(sap(v), brute_force_pairs(v)));
}
#endif

TEST(Broadphase, ColliderAabb) {
    const Aabb s = collider_aabb(Collider::sphere(2.0f), {1, 1, 1});
    EXPECT_EQ(s.min, glm::vec3(-1.0f));
    EXPECT_EQ(s.max, glm::vec3(3.0f));
    const Aabb b = collider_aabb(Collider::box({1, 2, 3}), {0, 0, 0});
    EXPECT_EQ(b.min, glm::vec3(-1, -2, -3));
    EXPECT_EQ(b.max, glm::vec3(1, 2, 3));
}

TEST(Broadphase, SeededRandomScenesMatchBruteForce) {
    std::mt19937 rng(12345);
    auto q = [&](int lo, int hi) {
        return float(int(rng() % unsigned(hi - lo + 1)) + lo) / 4.0f;
    };
    std::size_t total_pairs = 0;
    for (int scene = 0; scene < 100; ++scene) {
        const std::uint32_t n = rng() % 201;
        std::vector<BroadphaseProxy> v;
        for (std::uint32_t i = 0; i < n; ++i) {
            const glm::vec3 c(q(0, 40), q(0, 40), q(0, 40));
            const glm::vec3 h(q(0, 8), q(0, 8), q(0, 8));
            v.push_back(P(i, c - h, c + h, rng() % 10 < 3));
        }
        for (bool skip : {false, true}) {
            BroadphaseOptions o;
            o.skip_static_pairs = skip;
            const auto expect = brute_force_pairs(v, o);
            const auto got = sweep_and_prune(v, o);
            ASSERT_TRUE(same(expect, got)) << "scene " << scene << " skip " << skip;
            for (std::size_t i = 0; i < got.size(); ++i) {
                ASSERT_LT(got[i].a.index, got[i].b.index);
                if (i > 0) {
                    ASSERT_LT(std::tie(got[i - 1].a.index, got[i - 1].b.index),
                              std::tie(got[i].a.index, got[i].b.index));
                }
            }
            if (!skip) {
                total_pairs += got.size();
            }
            std::vector<BroadphaseProxy> shuffled = v;
            std::shuffle(shuffled.begin(), shuffled.end(), rng);
            ASSERT_TRUE(same(got, sweep_and_prune(shuffled, o)));
        }
    }
    EXPECT_GT(total_pairs, 1000u);
}

}  // namespace
}  // namespace engine::physics
