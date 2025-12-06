#include "engine/ecs/entity_pool.hpp"

#include <limits>
#include <stdexcept>

namespace engine {

Entity EntityPool::create() {
    std::uint32_t index = 0;
    if (!free_.empty()) {
        index = free_.back();
        free_.pop_back();
    } else {
        if (slots_.size() >= Entity::kInvalidIndex) {
            throw std::length_error("EntityPool::create: out of entity indices");
        }
        index = static_cast<std::uint32_t>(slots_.size());
        slots_.emplace_back();
    }
    Slot& slot = slots_[index];
    slot.alive = true;
    ++alive_count_;
    return Entity{index, slot.generation};
}

bool EntityPool::destroy(Entity e) {
    if (!alive(e)) {
        return false;
    }
    Slot& slot = slots_[e.index];
    slot.alive = false;
    --alive_count_;
    // Bump the generation so old handles stay invalid. A slot whose generation
    // is exhausted is retired instead of reused.
    if (slot.generation < std::numeric_limits<std::uint32_t>::max()) {
        ++slot.generation;
        free_.push_back(e.index);
    }
    return true;
}

bool EntityPool::alive(Entity e) const {
    return e.index < slots_.size() && slots_[e.index].alive &&
           slots_[e.index].generation == e.generation;
}

}  // namespace engine
