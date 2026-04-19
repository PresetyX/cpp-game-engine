#include <gtest/gtest.h>

#include "engine/assets/file_io.hpp"
#include "engine/assets/shader_source.hpp"
#include "fixtures.hpp"

using namespace engine::assets;

TEST(ShaderSource, LoadsBothStagesAndNormalizesLineEndings) {
    const auto src =
        load_shader_source(fixture("shaders/basic.vert"), fixture("shaders/basic.frag"));
    ASSERT_TRUE(src) << src.error();
    EXPECT_EQ(src->vertex.rfind("#version 330 core\n", 0), 0u);
    EXPECT_EQ(src->fragment.rfind("#version 330 core\n", 0), 0u);
    EXPECT_EQ(src->fragment.find('\r'), std::string::npos);  // fixture is stored with CRLF
}

TEST(ShaderSource, MissingFileFails) {
    EXPECT_FALSE(load_shader_source(fixture("shaders/basic.vert"), fixture("shaders/none.frag")));
    EXPECT_FALSE(load_shader_source(fixture("shaders/none.vert"), fixture("shaders/basic.frag")));
}

TEST(NormalizeText, StripsBomAndConvertsLineEndings) {
    EXPECT_EQ(normalize_text("\xEF\xBB\xBF"
                             "a\r\nb\rc\n"),
              "a\nb\nc\n");
    EXPECT_EQ(normalize_text(""), "");
}
