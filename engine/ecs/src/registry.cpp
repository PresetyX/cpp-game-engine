#include "engine/ecs/registry.hpp"

namespace engine {

bool Registry::destroy(Entity e) {
    if (!entities_.alive(e)) {
        return false;
    }
    for (auto& entry : pools_) {
        entry.second->remove(e);
    }
    return entities_.destroy(e);
}

}  // namespace engine
