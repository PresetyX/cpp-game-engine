#pragma once

#include <string_view>

namespace engine {

// Engine version as "major.minor.patch".
std::string_view version() noexcept;

}  // namespace engine
