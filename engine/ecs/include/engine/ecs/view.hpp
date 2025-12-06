#pragma once

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <vector>

#include "engine/ecs/entity.hpp"
#include "engine/ecs/sparse_set.hpp"

namespace engine {

namespace detail {
// A const T selects a read-only pool.
template <typename T>
using PoolFor = std::conditional_t<std::is_const_v<T>, const SparseSet<std::remove_const_t<T>>,
                                   SparseSet<T>>;
}  // namespace detail

// Entities that have all of Ts (each type at most once). Ts may be const for
// read-only access. Iterates the smallest pool backwards, so the body may
// remove components from, or destroy, the current entity. A View holds raw
// pool pointers: build it right before iterating and do not keep it across
// frames or across the creation of new component pools.
template <typename... Ts>
class View {
    static_assert(sizeof...(Ts) > 0, "View needs at least one component type");

public:
    class Iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = Entity;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = Entity;

        Iterator() = default;
        Iterator(const View* view, std::size_t pos) : view_(view), pos_(pos) { skip(); }

        Entity operator*() const { return (*view_->pivot_)[pos_ - 1]; }

        Iterator& operator++() {
            assert(pos_ > 0);
            --pos_;
            skip();
            return *this;
        }
        Iterator operator++(int) {
            Iterator copy = *this;
            ++*this;
            return copy;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) { return a.pos_ == b.pos_; }
        friend bool operator!=(const Iterator& a, const Iterator& b) { return !(a == b); }

    private:
        // Moves down to the next entity that has all components. The bound
        // check keeps this safe if the pivot shrank during iteration.
        void skip() {
            while (pos_ > 0 && (pos_ > view_->pivot_->size() ||
                                !view_->contains_all((*view_->pivot_)[pos_ - 1]))) {
                --pos_;
            }
        }

        const View* view_ = nullptr;
        std::size_t pos_ = 0;  // one past the current dense position; 0 is the end
    };

    // A null pool (component type never added) makes the view empty.
    explicit View(detail::PoolFor<Ts>*... pools) : pools_(pools...) {
        if ((... && (pools != nullptr))) {
            for (const std::vector<Entity>* candidate : {&pools->entities()...}) {
                if (pivot_ == nullptr || candidate->size() < pivot_->size()) {
                    pivot_ = candidate;
                }
            }
        }
    }

    Iterator begin() const { return pivot_ != nullptr ? Iterator(this, pivot_->size()) : end(); }
    Iterator end() const { return Iterator(this, 0); }

    // fn(Entity, Ts&...) or fn(Ts&...).
    template <typename F>
    void each(F&& fn) const {
        // The handle is copied: swap-and-pop may overwrite its slot during fn.
        for (const Entity e : *this) {
            if constexpr (std::is_invocable_v<F&, Entity, Ts&...>) {
                fn(e, pool<Ts>()->get(e)...);
            } else {
                fn(pool<Ts>()->get(e)...);
            }
        }
    }

    // T must be spelled exactly as in Ts (e.g. get<const Velocity>).
    // Precondition: e is in the view.
    template <typename T>
    decltype(auto) get(Entity e) const {
        assert(contains_all(e));
        return pool<T>()->get(e);
    }

    bool contains(Entity e) const { return pivot_ != nullptr && contains_all(e); }

    // Size of the pivot (smallest) pool: an upper bound on the entities visited.
    std::size_t size_hint() const { return pivot_ != nullptr ? pivot_->size() : 0; }

private:
    template <typename T>
    detail::PoolFor<T>* pool() const {
        return std::get<detail::PoolFor<T>*>(pools_);
    }

    bool contains_all(Entity e) const { return (... && pool<Ts>()->contains(e)); }

    std::tuple<detail::PoolFor<Ts>*...> pools_;
    const std::vector<Entity>* pivot_ = nullptr;
};

}  // namespace engine
