#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include "engine/ecs/entity.hpp"

namespace engine {

namespace detail {
// Braces for aggregates (C++17 has no parenthesized aggregate initialization).
template <typename T, typename... Args>
T make_component(Args&&... args) {
    if constexpr (std::is_aggregate_v<T>) {
        return T{std::forward<Args>(args)...};
    } else {
        return T(std::forward<Args>(args)...);
    }
}
}  // namespace detail

// Type-erased interface so the registry can drop every component of an entity.
class SparseSetBase {
public:
    virtual ~SparseSetBase() = default;
    virtual bool remove(Entity e) = 0;
    virtual bool contains(Entity e) const = 0;
    virtual std::size_t size() const = 0;
};

// Components of one type in a dense array. sparse_[entity.index] holds the
// position in dense_/values_, or kNone. Removal is swap-and-pop, so iteration
// order is unspecified and references/pointers to components are invalidated
// by any insert or remove.
template <typename T>
class SparseSet final : public SparseSetBase {
    static_assert(std::is_same_v<T, std::decay_t<T>>,
                  "component type must not be const, a reference or an array");
    static_assert(!std::is_same_v<T, bool>, "wrap bool in a struct (std::vector<bool>)");
    static_assert(std::is_move_constructible_v<T> && std::is_move_assignable_v<T>,
                  "components must be move constructible and move assignable");

public:
    bool contains(Entity e) const override {
        if (e.index >= sparse_.size()) {
            return false;
        }
        const std::uint32_t pos = sparse_[e.index];
        return pos != kNone && dense_[pos] == e;
    }

    // Adds the component of e, or replaces it if e already has one.
    // Precondition: no other generation of e.index is stored (the registry
    // guarantees this by removing components on destroy).
    template <typename... Args>
    T& emplace(Entity e, Args&&... args) {
        assert(!e.is_null());
        T value = detail::make_component<T>(std::forward<Args>(args)...);
        if (contains(e)) {
            T& slot = values_[sparse_[e.index]];
            slot = std::move(value);
            return slot;
        }
        if (e.index >= sparse_.size()) {
            sparse_.resize(static_cast<std::size_t>(e.index) + 1, kNone);
        }
        dense_.push_back(e);
        try {
            values_.push_back(std::move(value));
        } catch (...) {
            dense_.pop_back();
            throw;
        }
        sparse_[e.index] = static_cast<std::uint32_t>(dense_.size() - 1);
        return values_.back();
    }

    // Swap-and-pop: the last element moves into the removed slot.
    bool remove(Entity e) override {
        if (!contains(e)) {
            return false;
        }
        const std::size_t pos = sparse_[e.index];
        const std::size_t last = dense_.size() - 1;
        if (pos != last) {
            dense_[pos] = dense_[last];
            values_[pos] = std::move(values_[last]);
            sparse_[dense_[pos].index] = static_cast<std::uint32_t>(pos);
        }
        dense_.pop_back();
        values_.pop_back();
        sparse_[e.index] = kNone;  // last: e.index may equal the moved entity's index
        return true;
    }

    // Unchecked in release builds: precondition contains(e).
    T& get(Entity e) {
        assert(contains(e));
        return values_[sparse_[e.index]];
    }
    const T& get(Entity e) const {
        assert(contains(e));
        return values_[sparse_[e.index]];
    }

    T* try_get(Entity e) { return contains(e) ? &values_[sparse_[e.index]] : nullptr; }
    const T* try_get(Entity e) const {
        return contains(e) ? &values_[sparse_[e.index]] : nullptr;
    }

    std::size_t size() const override { return dense_.size(); }
    bool empty() const { return dense_.empty(); }
    // entities()[i] owns data()[i].
    const std::vector<Entity>& entities() const { return dense_; }
    const std::vector<T>& data() const { return values_; }

    void clear() {
        sparse_.clear();
        dense_.clear();
        values_.clear();
    }

private:
    static constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();

    std::vector<std::uint32_t> sparse_;
    std::vector<Entity> dense_;
    std::vector<T> values_;
};

}  // namespace engine
