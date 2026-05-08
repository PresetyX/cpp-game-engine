#include <gtest/gtest.h>

#include <memory>

#include "engine/assets/asset_manager.hpp"
#include "engine/render/image_upload.hpp"
#include "engine/render/upload_cache.hpp"
#include "fixtures.hpp"

using engine::assets::Handle;
using engine::assets::Image;
using engine::render::flip_rows;
using engine::render::UploadCache;

namespace {

struct Tag {};
using H = Handle<Tag>;

// Counts live instances, to check that GPU objects are destroyed.
struct Gpu {
    explicit Gpu(int v, int* live) : value(v), live_count(live) { ++*live_count; }
    Gpu(Gpu&& o) noexcept : value(o.value), live_count(o.live_count) { ++*live_count; }
    Gpu& operator=(Gpu&& o) noexcept {
        value = o.value;
        return *this;
    }
    Gpu(const Gpu&) = delete;
    ~Gpu() { --*live_count; }
    int value;
    int* live_count;
};

}  // namespace

TEST(UploadCache, FirstUseNeedsUploadThenNot) {
    int live = 0;
    UploadCache<H, Gpu> cache;
    const H h{3, 1};
    EXPECT_TRUE(cache.needs_upload(h, 1));
    cache.store(h, 1, Gpu(10, &live));
    EXPECT_FALSE(cache.needs_upload(h, 1));
    ASSERT_NE(cache.find(h), nullptr);
    EXPECT_EQ(cache.find(h)->value, 10);
    EXPECT_EQ(cache.stored_version(h), 1u);
}

TEST(UploadCache, NewVersionNeedsUploadAndReplacesOldObject) {
    int live = 0;
    UploadCache<H, Gpu> cache;
    const H h{0, 1};
    cache.store(h, 1, Gpu(1, &live));
    EXPECT_TRUE(cache.needs_upload(h, 2));
    cache.store(h, 2, Gpu(2, &live));
    EXPECT_EQ(live, 1);
    EXPECT_EQ(cache.size(), 1u);
    EXPECT_EQ(cache.find(h)->value, 2);
    EXPECT_FALSE(cache.needs_upload(h, 2));
}

TEST(UploadCache, VersionZeroNeverUploads) {
    UploadCache<H, int> cache;
    EXPECT_FALSE(cache.needs_upload(H{0, 1}, 0));
}

TEST(UploadCache, CollectStaleErasesReleasedAssetsOnly) {
    int live = 0;
    UploadCache<H, Gpu> cache;
    const H a{0, 1};
    const H b{1, 1};
    cache.store(a, 1, Gpu(1, &live));
    cache.store(b, 1, Gpu(2, &live));
    const std::size_t erased = cache.collect_stale([&](H h) { return h == a ? 0u : 1u; });
    EXPECT_EQ(erased, 1u);
    EXPECT_EQ(cache.find(a), nullptr);
    EXPECT_NE(cache.find(b), nullptr);
    EXPECT_EQ(live, 1);
}

TEST(UploadCache, ReusedSlotWithNewGenerationIsADifferentEntry) {
    int live = 0;
    UploadCache<H, Gpu> cache;
    const H old_handle{5, 1};
    const H new_handle{5, 2};
    cache.store(old_handle, 1, Gpu(1, &live));
    EXPECT_TRUE(cache.needs_upload(new_handle, 1));
    // The old asset is gone (version 0), the new one is live.
    cache.store(new_handle, 1, Gpu(2, &live));
    cache.collect_stale([&](H h) { return h == old_handle ? 0u : 1u; });
    EXPECT_EQ(cache.size(), 1u);
    EXPECT_EQ(cache.find(new_handle)->value, 2);
    EXPECT_EQ(live, 1);
}

TEST(UploadCache, ClearDestroysEverything) {
    int live = 0;
    {
        UploadCache<H, Gpu> cache;
        cache.store(H{0, 1}, 1, Gpu(1, &live));
        cache.store(H{1, 1}, 1, Gpu(2, &live));
        cache.clear();
        EXPECT_EQ(live, 0);
        cache.store(H{2, 1}, 1, Gpu(3, &live));
    }
    EXPECT_EQ(live, 0);
}

// Hot reload end to end on the CPU side: the AssetManager version drives the cache.
TEST(UploadCache, FollowsAssetManagerVersion) {
    engine::assets::AssetManager assets(fixture_root());
    const auto loaded = assets.load<Image>("test_2x2.png");
    ASSERT_TRUE(loaded.ok()) << loaded.error();
    const auto h = loaded.value();

    UploadCache<Handle<Image>, int> cache;
    ASSERT_TRUE(cache.needs_upload(h, assets.version(h)));
    cache.store(h, assets.version(h), 1);
    EXPECT_FALSE(cache.needs_upload(h, assets.version(h)));

    assets.release(h);
    EXPECT_EQ(assets.version(h), 0u);
    EXPECT_EQ(cache.collect_stale([&](Handle<Image> x) { return assets.version(x); }), 1u);
}

TEST(FlipRows, ReversesRowOrderKeepingPixelsIntact) {
    Image img;
    img.width = 2;
    img.height = 3;
    for (std::uint8_t row = 0; row < 3; ++row) {
        for (std::uint8_t x = 0; x < 2; ++x) {
            for (std::uint8_t c = 0; c < 4; ++c) {
                img.pixels.push_back(static_cast<std::uint8_t>(row * 10 + x * 4 + c));
            }
        }
    }
    const auto flipped = flip_rows(img);
    ASSERT_EQ(flipped.size(), img.pixels.size());
    // First output row is the last input row (row 2: values 20..27).
    for (std::uint8_t i = 0; i < 8; ++i) {
        EXPECT_EQ(flipped[i], 20 + i);
    }
    // Middle row unchanged, last output row is the first input row.
    for (std::uint8_t i = 0; i < 8; ++i) {
        EXPECT_EQ(flipped[8 + i], 10 + i);
        EXPECT_EQ(flipped[16 + i], i);
    }
}

TEST(FlipRows, FlippingTwiceIsIdentity) {
    Image img;
    img.width = 3;
    img.height = 4;
    for (int i = 0; i < 3 * 4 * 4; ++i) {
        img.pixels.push_back(static_cast<std::uint8_t>(i * 7));
    }
    Image once = img;
    once.pixels = flip_rows(img);
    EXPECT_EQ(flip_rows(once), img.pixels);
}

TEST(FlipRows, InvalidImageGivesEmpty) {
    Image img;
    EXPECT_TRUE(flip_rows(img).empty());
    img.width = 2;
    img.height = 2;
    img.pixels.resize(3);  // too small
    EXPECT_TRUE(flip_rows(img).empty());
}
