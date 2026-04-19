#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

#include "engine/assets/asset_manager.hpp"
#include "fixtures.hpp"

using namespace engine::assets;
namespace fs = std::filesystem;

namespace {

// Copy of the fixtures in a scratch directory, so reload tests can modify files.
class AssetManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = fs::temp_directory_path() /
               (std::string("engine_asset_test_") +
                ::testing::UnitTest::GetInstance()->current_test_info()->name());
        fs::remove_all(dir_);
        fs::create_directories(dir_);
        fs::copy(fixture_root(), dir_, fs::copy_options::recursive);
    }
    void TearDown() override { fs::remove_all(dir_); }

    void write(const std::string& rel, const std::string& text) {
        const fs::path p = dir_ / rel;
        const auto old_time = fs::last_write_time(p);
        {
            std::ofstream f(p, std::ios::binary | std::ios::trunc);
            f << text;
        }
        // Do not rely on the clock resolution: force a different modification time.
        fs::last_write_time(p, old_time + std::chrono::seconds(5));
    }

    fs::path dir_;
};

}  // namespace

TEST_F(AssetManagerTest, LoadsAndReturnsData) {
    AssetManager am(dir_);
    const auto h = am.load<Image>("test_2x2.png");
    ASSERT_TRUE(h) << h.error();
    ASSERT_NE(am.get(*h), nullptr);
    EXPECT_EQ(am.get(*h)->width, 2);
    EXPECT_EQ(am.version(*h), 1u);
    EXPECT_EQ(am.size<Image>(), 1u);
}

TEST_F(AssetManagerTest, SecondLoadHitsTheCache) {
    AssetManager am(dir_);
    const auto a = am.load<Mesh>("quad.obj");
    const auto b = am.load<Mesh>("./sub/../quad.obj");  // same file, different spelling
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    EXPECT_EQ(*a, *b);
    EXPECT_EQ(am.size<Mesh>(), 1u);
    EXPECT_EQ(am.ref_count(*a), 2u);
}

TEST_F(AssetManagerTest, ReleaseFreesAtZeroAndHandleGoesStale) {
    AssetManager am(dir_);
    const auto a = *am.load<Image>("test_2x2.png");
    ASSERT_TRUE(am.load<Image>("test_2x2.png"));
    am.release(a);
    EXPECT_NE(am.get(a), nullptr);
    am.release(a);
    EXPECT_EQ(am.get(a), nullptr);
    EXPECT_EQ(am.version(a), 0u);
    EXPECT_EQ(am.size<Image>(), 0u);
    am.release(a);  // releasing a stale handle is a no-op

    const auto again = *am.load<Image>("test_2x2.png");  // reuses the slot, new generation
    EXPECT_EQ(again.index, a.index);
    EXPECT_NE(again.generation, a.generation);
    EXPECT_EQ(am.get(a), nullptr);
    EXPECT_NE(am.get(again), nullptr);
}

TEST_F(AssetManagerTest, FailedLoadReportsErrorAndCachesNothing) {
    AssetManager am(dir_);
    const auto h = am.load<Image>("missing.png");
    EXPECT_FALSE(h);
    EXPECT_FALSE(h.error().empty());
    EXPECT_FALSE(am.load<Image>("corrupt.png"));
    EXPECT_FALSE(am.load<Image>(""));
    EXPECT_EQ(am.size<Image>(), 0u);
}

TEST_F(AssetManagerTest, InvalidHandleResolvesToNull) {
    AssetManager am(dir_);
    EXPECT_EQ(am.get(Handle<Image>{}), nullptr);
    EXPECT_EQ(am.get(Handle<Mesh>{}), nullptr);
}

TEST_F(AssetManagerTest, DifferentTypesDoNotShareKeys) {
    AssetManager am(dir_);
    ASSERT_TRUE(am.load<Mesh>("quad.obj"));
    EXPECT_FALSE(am.load<Image>("quad.obj"));  // not a PNG
}

TEST_F(AssetManagerTest, ShaderLoadsFromBasePath) {
    AssetManager am(dir_);
    const auto h = am.load<ShaderSource>("shaders/basic");
    ASSERT_TRUE(h) << h.error();
    EXPECT_NE(am.get(*h)->vertex.find("a_pos"), std::string::npos);
}

TEST_F(AssetManagerTest, PointerStaysValidWhenLoadingOtherAssets) {
    AssetManager am(dir_);
    const auto a = *am.load<Image>("test_2x2.png");
    const Image* p = am.get(a);
    for (int i = 0; i < 50; ++i) {
        const std::string name = "c" + std::to_string(i) + ".png";
        fs::copy_file(dir_ / "test_2x2.png", dir_ / name);
        ASSERT_TRUE(am.load<Image>(name));
    }
    EXPECT_EQ(am.get(a), p);
}

TEST_F(AssetManagerTest, ReloadChangedPicksUpEditedFile) {
    AssetManager am(dir_);
    const auto h = *am.load<Mesh>("quad.obj");
    EXPECT_EQ(am.reload_changed(), 0u);  // nothing changed

    write("quad.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
    EXPECT_EQ(am.reload_changed(), 1u);
    EXPECT_EQ(am.get(h)->indices.size(), 3u);
    EXPECT_EQ(am.version(h), 2u);
    EXPECT_EQ(am.reload_changed(), 0u);  // already up to date
}

TEST_F(AssetManagerTest, ShaderReloadWatchesBothFiles) {
    AssetManager am(dir_);
    const auto h = *am.load<ShaderSource>("shaders/basic");
    write("shaders/basic.frag", "#version 330 core\nvoid main() {}\n");
    EXPECT_EQ(am.reload_changed(), 1u);
    EXPECT_EQ(am.get(h)->fragment, "#version 330 core\nvoid main() {}\n");
}

TEST_F(AssetManagerTest, FailedReloadKeepsOldDataAndDoesNotRetryUntilChanged) {
    AssetManager am(dir_);
    const auto h = *am.load<Mesh>("quad.obj");
    write("quad.obj", "f 1 2 3\n");  // invalid: indices out of range
    EXPECT_EQ(am.reload_changed(), 0u);
    EXPECT_EQ(am.get(h)->indices.size(), 6u);  // old mesh kept
    EXPECT_EQ(am.version(h), 1u);
    EXPECT_EQ(am.reload_changed(), 0u);

    write("quad.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");  // fixed
    EXPECT_EQ(am.reload_changed(), 1u);
    EXPECT_EQ(am.get(h)->indices.size(), 3u);
}

TEST_F(AssetManagerTest, ReleasedAssetsAreNotReloaded) {
    AssetManager am(dir_);
    const auto h = *am.load<Mesh>("quad.obj");
    am.release(h);
    write("quad.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
    EXPECT_EQ(am.reload_changed(), 0u);
}
