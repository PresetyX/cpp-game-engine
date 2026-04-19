#include <gtest/gtest.h>

#include "engine/assets/mesh.hpp"
#include "fixtures.hpp"

using namespace engine::assets;

TEST(Obj, LoadsQuadFixtureAsTwoTriangles) {
    const auto mesh = load_obj(fixture("quad.obj"));
    ASSERT_TRUE(mesh) << mesh.error();
    EXPECT_EQ(mesh->vertices.size(), 4u);
    ASSERT_EQ(mesh->indices.size(), 6u);
    const std::vector<std::uint32_t> expected{0, 1, 2, 0, 2, 3};
    EXPECT_EQ(mesh->indices, expected);
    EXPECT_EQ(mesh->vertices[2].position, glm::vec3(1, 1, 0));
    EXPECT_EQ(mesh->vertices[2].uv, glm::vec2(1, 1));
    EXPECT_EQ(mesh->vertices[2].normal, glm::vec3(0, 0, 1));
}

TEST(Obj, SharedCornersAreDeduplicated) {
    const auto mesh = parse_obj("v 0 0 0\nv 1 0 0\nv 0 1 0\nv 1 1 0\nf 1 2 3\nf 2 4 3\n");
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->vertices.size(), 4u);
    EXPECT_EQ(mesh->indices.size(), 6u);
}

TEST(Obj, SamePositionWithDifferentUvStaysSeparate) {
    const auto mesh = parse_obj(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvt 1 1\nf 1/1 2/1 3/1\nf 1/2 2/1 3/1\n");
    ASSERT_TRUE(mesh);
    EXPECT_EQ(mesh->vertices.size(), 4u);
}

TEST(Obj, MissingNormalsAreComputed) {
    const auto mesh = parse_obj("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
    ASSERT_TRUE(mesh);
    for (const Vertex& v : mesh->vertices) {
        EXPECT_NEAR(v.normal.z, 1.0f, 1e-6f);
    }
}

TEST(Obj, NegativeIndicesAndNormalOnlyFormat) {
    const auto mesh =
        parse_obj("v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 -1\nf -3//-1 -2//-1 -1//-1\n");
    ASSERT_TRUE(mesh) << mesh.error();
    EXPECT_EQ(mesh->vertices[1].position, glm::vec3(1, 0, 0));
    EXPECT_EQ(mesh->vertices[1].normal, glm::vec3(0, 0, -1));
}

TEST(Obj, HandlesCrlfCommentsAndUnknownStatements) {
    const auto mesh = parse_obj(
        "# c\r\nmtllib x.mtl\r\nv 0 0 0\r\nv 1 0 0\r\nv 0 1 0\r\ns off\r\nf 1 2 3\r\n");
    ASSERT_TRUE(mesh) << mesh.error();
    EXPECT_EQ(mesh->indices.size(), 3u);
}

TEST(Obj, ScientificNotation) {
    const auto mesh = parse_obj("v 1e-1 -2.5E+1 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
    ASSERT_TRUE(mesh);
    EXPECT_FLOAT_EQ(mesh->vertices[0].position.x, 0.1f);
    EXPECT_FLOAT_EQ(mesh->vertices[0].position.y, -25.0f);
}

TEST(Obj, ErrorsReportTheLine) {
    const auto out_of_range = parse_obj("v 0 0 0\nv 1 0 0\nf 1 2 3\n");
    ASSERT_FALSE(out_of_range);
    EXPECT_NE(out_of_range.error().find("line 3"), std::string::npos);

    EXPECT_FALSE(parse_obj("v 0 0\nf 1 1 1\n"));         // too few coordinates
    EXPECT_FALSE(parse_obj("v 0 0 x\n"));                 // not a number
    EXPECT_FALSE(parse_obj("v 0 0 0\nf 1 1\n"));          // face with 2 vertices
    EXPECT_FALSE(parse_obj("v 0 0 0\nf 0 0 0\n"));        // index 0 is invalid
    EXPECT_FALSE(parse_obj("v 0 0 0\nf 1/1/1/1 1 1\n"));  // too many slashes
    EXPECT_FALSE(parse_obj(""));                          // no faces
}

TEST(Obj, MissingFileFails) {
    EXPECT_FALSE(load_obj(fixture("nope.obj")));
}
