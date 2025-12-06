#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>

#include "engine/ecs/entity.hpp"
#include "engine/ecs/entity_pool.hpp"
#include "engine/ecs/sparse_set.hpp"
#include "engine/ecs/view.hpp"

namespace engine {

// Owns entities and their components (one SparseSet per component type).
// Single-threaded by design. Component references are invalidated by any
// add/remove of the same component type.
//
// While iterating a view it is safe to modify component values, create
// entities (not visited), add or remove components of, or destroy, the
// *current* entity. Changing the viewed pools for other entities is
// memory-safe but may skip or repeat entities: defer those changes.
class Registry {
public:
    Entity create() { return entities_.create(); }

    // Destroys e and removes all its components. No-op (false) if e is not alive.
    bool destroy(Entity e);

    bool alive(Entity e) const { return entities_.alive(e); }
    std::size_t alive_count() const { return entities_.alive_count(); }
    std::size_t slot_count() const { return entities_.slot_count(); }

    // Adds T to e, or replaces it. Throws std::invalid_argument if e is not alive.
    template <typename T, typename... Args>
    T& add(Entity e, Args&&... args) {
        if (!alive(e)) {
            throw std::invalid_argument("Registry::add: entity is not alive");
        }
        return assure<T>().emplace(e, std::forward<Args>(args)...);
    }

    template <typename T>
    bool remove(Entity e) {
        SparseSet<T>* pool = find_pool<T>();
        return pool != nullptr && pool->remove(e);
    }

    template <typename T>
    bool has(Entity e) const {
        const SparseSet<T>* pool = find_pool<T>();
        return pool != nullptr && pool->contains(e);
    }

    template <typename T>
    T* try_get(Entity e) {
        SparseSet<T>* pool = find_pool<T>();
        return pool != nullptr ? pool->try_get(e) : nullptr;
    }
    template <typename T>
    const T* try_get(Entity e) const {
        const SparseSet<T>* pool = find_pool<T>();
        return pool != nullptr ? pool->try_get(e) : nullptr;
    }

    // Precondition: has<T>(e). Checked with assert only.
    template <typename T>
    T& get(Entity e) {
        T* component = try_get<T>(e);
        assert(component != nullptr);
        return *component;
    }
    template <typename T>
    const T& get(Entity e) const {
        const T* component = try_get<T>(e);
        assert(component != nullptr);
        return *component;
    }

    template <typename... Ts>
    View<Ts...> view() {
        return View<Ts...>(find_pool<std::remove_const_t<Ts>>()...);
    }
    template <typename... Ts>
    View<const Ts...> view() const {
        return View<const Ts...>(find_pool<std::remove_const_t<Ts>>()...);
    }

private:
    template <typename T>
    SparseSet<T>& assure() {
        auto& slot = pools_[std::type_index(typeid(T))];
        if (!slot) {
            slot = std::make_unique<SparseSet<T>>();
        }
        return static_cast<SparseSet<T>&>(*slot);
    }

    template <typename T>
    SparseSet<T>* find_pool() {
        auto it = pools_.find(std::type_index(typeid(T)));
        return it == pools_.end() ? nullptr : static_cast<SparseSet<T>*>(it->second.get());
    }
    template <typename T>
    const SparseSet<T>* find_pool() const {
        auto it = pools_.find(std::type_index(typeid(T)));
        return it == pools_.end() ? nullptr : static_cast<const SparseSet<T>*>(it->second.get());
    }

    EntityPool entities_;
    std::unordered_map<std::type_index, std::unique_ptr<SparseSetBase>> pools_;
};

}  // namespace engine
