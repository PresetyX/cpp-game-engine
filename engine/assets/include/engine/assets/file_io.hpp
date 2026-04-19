#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "engine/assets/result.hpp"

namespace engine::assets {

LoadResult<std::vector<std::uint8_t>> read_binary_file(const std::filesystem::path& path);

// Raw text of the file (bytes unchanged).
LoadResult<std::string> read_text_file(const std::filesystem::path& path);

// Strips a UTF-8 BOM and converts CRLF / lone CR to LF, so parsers and GLSL (`#version` must
// be the first thing in the source) see the same text on every platform.
std::string normalize_text(std::string text);

}  // namespace engine::assets
