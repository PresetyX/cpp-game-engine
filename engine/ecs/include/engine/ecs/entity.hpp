#pragma once

#include <cstdint>
#include <limits>

namespace engine {

// Entity handle: slot index plus generation. A handle is alive only while its
// generation matches the slot; destroying an entity bumps the generation, so
// stale handles to a reused index are rejected.
struct Entity {
    static constexpr std::uint32_t kInvalidIndex = std::numeric_limits<std::uint32_t>::max();

    std::uint32_t index = kInvalidIndex;
    std::uint32_t generation = 0;

    constexpr bool is_null() const { return index == kInvalidIndex; }
};

constexpr bool operator==(Entity a, Entity b) {
    return a.index == b.index && a.generation == b.generation;
}
constexpr bool operator!=(Entity a, Entity b) {
    return !(a == b);
}

inline constexpr Entity null_entity{};

}  // namespace engine
