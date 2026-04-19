#include <gtest/gtest.h>

#include "engine/assets/image.hpp"
#include "fixtures.hpp"

using namespace engine::assets;

TEST(Image, LoadsPngAsRgba) {
    const auto img = load_image(fixture("test_2x2.png"));
    ASSERT_TRUE(img) << img.error();
    EXPECT_EQ(img->width, 2);
    EXPECT_EQ(img->height, 2);
    ASSERT_EQ(img->pixels.size(), 16u);
    const std::uint8_t* red = img->pixel(0, 0);
    EXPECT_EQ(red[0], 255);
    EXPECT_EQ(red[1], 0);
    EXPECT_EQ(red[2], 0);
    EXPECT_EQ(red[3], 255);
    EXPECT_EQ(img->pixel(1, 1)[3], 128);
}

TEST(Image, FlipVerticalSwapsRows) {
    ImageOptions opt;
    opt.flip_vertical = true;
    const auto img = load_image(fixture("test_2x2.png"), opt);
    ASSERT_TRUE(img);
    EXPECT_EQ(img->pixel(0, 0)[2], 255);  // blue was at the bottom-left
    EXPECT_EQ(img->pixel(0, 1)[0], 255);  // red moved to the bottom-left
}

TEST(Image, MissingFileFails) {
    const auto img = load_image(fixture("does_not_exist.png"));
    EXPECT_FALSE(img);
    EXPECT_NE(img.error().find("does_not_exist.png"), std::string::npos);
}

TEST(Image, CorruptFileFails) {
    const auto img = load_image(fixture("corrupt.png"));
    EXPECT_FALSE(img);
    EXPECT_FALSE(img.error().empty());
}

TEST(Image, EmptyInputFails) {
    EXPECT_FALSE(decode_image(nullptr, 0));
}
