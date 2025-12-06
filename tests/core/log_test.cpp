#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "engine/core/log.hpp"

using engine::LogLevel;

namespace {

struct Captured {
    LogLevel level;
    std::string message;
};

// Installs a capturing sink and restores global logger state afterwards.
class LogTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine::set_log_level(LogLevel::Trace);
        engine::set_log_sink([this](LogLevel level, std::string_view message) {
            captured.push_back({level, std::string(message)});
        });
    }
    void TearDown() override {
        engine::set_log_sink({});
        engine::set_log_level(LogLevel::Info);
    }

    std::vector<Captured> captured;
};

}  // namespace

TEST_F(LogTest, SinkReceivesLevelAndMessage) {
    engine::log_warn("disk low");
    ASSERT_EQ(captured.size(), 1u);
    EXPECT_EQ(captured[0].level, LogLevel::Warn);
    EXPECT_EQ(captured[0].message, "disk low");
}

TEST_F(LogTest, ArgumentsAreConcatenated) {
    engine::log_info("x=", 3, " y=", 1.5, " ok=", true);
    ASSERT_EQ(captured.size(), 1u);
    EXPECT_EQ(captured[0].message, "x=3 y=1.5 ok=1");
}

TEST_F(LogTest, MessagesBelowLevelAreDiscarded) {
    engine::set_log_level(LogLevel::Warn);
    engine::log_debug("hidden");
    engine::log_info("hidden");
    engine::log_warn("shown");
    engine::log_error("shown");
    ASSERT_EQ(captured.size(), 2u);
    EXPECT_EQ(captured[0].level, LogLevel::Warn);
    EXPECT_EQ(captured[1].level, LogLevel::Error);
}

TEST_F(LogTest, LevelRoundTrips) {
    engine::set_log_level(LogLevel::Error);
    EXPECT_EQ(engine::log_level(), LogLevel::Error);
}

TEST_F(LogTest, DefaultSinkDoesNotCrash) {
    engine::set_log_sink({});
    engine::log_info("goes to stderr");
    SUCCEED();
}
