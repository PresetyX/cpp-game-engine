#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "engine/ecs/entity.hpp"

namespace engine {

// Hands out entity handles. One slot per index (generation + alive flag) and a
// LIFO free list of destroyed indices. It knows nothing about components.
class EntityPool {
public:
    // New entity; reuses the most recently freed index with a newer generation.
    Entity create();

    // Returns false (and does nothing) if e is not alive.
    bool destroy(Entity e);

    bool alive(Entity e) const;

    std::size_t alive_count() const { return alive_count_; }
    // Number of slots ever allocated (alive + free + retired).
    std::size_t slot_count() const { return slots_.size(); }

private:
    struct Slot {
        std::uint32_t generation = 0;
        bool alive = false;
    };

    std::vector<Slot> slots_;
    std::vector<std::uint32_t> free_;
    std::size_t alive_count_ = 0;
};

}  // namespace engine
