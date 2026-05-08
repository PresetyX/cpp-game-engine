#include <gtest/gtest.h>

#include "engine/render/shader_log.hpp"

using engine::render::DiagnosticSeverity;
using engine::render::format_diagnostics;
using engine::render::parse_info_log;

TEST(ShaderLog, EmptyLogGivesNothing) {
    EXPECT_TRUE(parse_info_log("").empty());
    EXPECT_TRUE(parse_info_log("\n\r\n  \n").empty());
}

TEST(ShaderLog, Nvidia) {
    const auto d = parse_info_log("0(12) : error C1008: undefined variable \"foo\"\n");
    ASSERT_EQ(d.size(), 1u);
    EXPECT_EQ(d[0].severity, DiagnosticSeverity::Error);
    EXPECT_EQ(d[0].source, 0);
    EXPECT_EQ(d[0].line, 12);
    EXPECT_EQ(d[0].message, "C1008: undefined variable \"foo\"");
}

TEST(ShaderLog, NvidiaWarning) {
    const auto d = parse_info_log("0(3) : warning C7050: \"x\" might be used before being set\n");
    ASSERT_EQ(d.size(), 1u);
    EXPECT_EQ(d[0].severity, DiagnosticSeverity::Warning);
    EXPECT_EQ(d[0].line, 3);
}

TEST(ShaderLog, MesaWithColumn) {
    const auto d = parse_info_log("0:12(5): error: 'foo' undeclared\n");
    ASSERT_EQ(d.size(), 1u);
    EXPECT_EQ(d[0].severity, DiagnosticSeverity::Error);
    EXPECT_EQ(d[0].line, 12);
    EXPECT_EQ(d[0].message, "'foo' undeclared");
}

TEST(ShaderLog, MesaWithoutColumn) {
    const auto d = parse_info_log("0:7: warning: unused\n");
    ASSERT_EQ(d.size(), 1u);
    EXPECT_EQ(d[0].severity, DiagnosticSeverity::Warning);
    EXPECT_EQ(d[0].line, 7);
    EXPECT_EQ(d[0].message, "unused");
}

TEST(ShaderLog, IntelAmdPrefixed) {
    const auto d = parse_info_log("ERROR: 0:31: 'foo' : undeclared identifier\r\n");
    ASSERT_EQ(d.size(), 1u);
    EXPECT_EQ(d[0].severity, DiagnosticSeverity::Error);
    EXPECT_EQ(d[0].source, 0);
    EXPECT_EQ(d[0].line, 31);
    EXPECT_EQ(d[0].message, "'foo' : undeclared identifier");
}

TEST(ShaderLog, MultipleMessagesKeepOrderAndMixedDialects) {
    const auto d = parse_info_log(
        "0(1) : error C0000: first\n"
        "ERROR: 0:2: second\n"
        "0:3(1): error: third\n");
    ASSERT_EQ(d.size(), 3u);
    EXPECT_EQ(d[0].line, 1);
    EXPECT_EQ(d[1].line, 2);
    EXPECT_EQ(d[2].line, 3);
}

TEST(ShaderLog, UnknownLinesAreKeptAsNotesWithoutLine) {
    const auto d = parse_info_log("Vertex shader failed to compile with the following errors:\n");
    ASSERT_EQ(d.size(), 1u);
    EXPECT_EQ(d[0].severity, DiagnosticSeverity::Note);
    EXPECT_EQ(d[0].line, 0);
    EXPECT_EQ(d[0].message, "Vertex shader failed to compile with the following errors:");
}

TEST(ShaderLog, LinkerMessageWithoutPosition) {
    const auto d = parse_info_log("error: unresolved symbol foo\n");
    ASSERT_EQ(d.size(), 1u);
    EXPECT_EQ(d[0].line, 0);
    EXPECT_EQ(d[0].message, "error: unresolved symbol foo");
}

TEST(ShaderLog, TrailingNulAndNoNewlineAreHandled) {
    const std::string log("0(5) : error C1: x\0", 19);
    const auto d = parse_info_log(log);
    ASSERT_EQ(d.size(), 1u);
    EXPECT_EQ(d[0].line, 5);
    EXPECT_EQ(d[0].message, "C1: x");
}

TEST(ShaderLog, GarbageDoesNotCrash) {
    for (const char* s : {"0(", "0(1", "0(1)", "0(1) :", "0:", "0:1(", "ERROR:", "ERROR: 0:",
                          "99999999999999999999(1) : error", ":::", "(((("}) {
        EXPECT_NO_THROW({ (void)parse_info_log(s); });
    }
}

TEST(ShaderLog, FormatUsesFileAndLine) {
    const auto d = parse_info_log("0(12) : error C1008: bad\n0:3(1): warning: meh\n");
    EXPECT_EQ(format_diagnostics(d, "shaders/phong.frag"),
              "shaders/phong.frag:12: error: C1008: bad\n"
              "shaders/phong.frag:3: warning: meh\n");
}

TEST(ShaderLog, FormatWithoutLineAndNote) {
    const auto d = parse_info_log("something odd\n");
    EXPECT_EQ(format_diagnostics(d, "a.vert"), "a.vert: something odd\n");
}
