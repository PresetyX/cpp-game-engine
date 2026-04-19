#pragma once

#include <filesystem>
#include <string>

// Small files under tests/assets/fixtures, found through a compile-time path.
inline std::filesystem::path fixture_root() {
    return std::filesystem::path(ENGINE_TEST_FIXTURE_DIR);
}
inline std::filesystem::path fixture(const std::string& name) {
    return fixture_root() / name;
}
