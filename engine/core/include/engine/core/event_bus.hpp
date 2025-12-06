#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine {

using SubscriptionId = std::uint64_t;

// Typed event bus. publish() dispatches immediately; enqueue() defers delivery
// until flush(). Handlers may subscribe/unsubscribe while an event is being
// dispatched: removals take effect immediately, additions from the next publish.
class EventBus {
public:
    template <typename E>
    SubscriptionId subscribe(std::function<void(const E&)> handler) {
        const SubscriptionId id = next_id_++;
        list<E>().handlers.push_back({id, std::move(handler), true});
        return id;
    }

    void unsubscribe(SubscriptionId id) {
        for (auto& entry : lists_) {
            if (entry.second->remove(id)) {
                return;
            }
        }
    }

    template <typename E>
    void publish(const E& event) {
        auto it = lists_.find(std::type_index(typeid(E)));
        if (it == lists_.end()) {
            return;
        }
        auto& handlers = static_cast<HandlerList<E>&>(*it->second);
        ++handlers.depth;
        // Index loop with a fixed bound: handlers added during dispatch are skipped.
        const std::size_t count = handlers.handlers.size();
        for (std::size_t i = 0; i < count; ++i) {
            // Copy: the callable may be invalidated if the vector reallocates.
            const auto handler = handlers.handlers[i];
            if (handler.alive) {
                handler.fn(event);
            }
        }
        if (--handlers.depth == 0) {
            handlers.compact();
        }
    }

    template <typename E>
    void enqueue(E event) {
        queue_.push_back([this, event = std::move(event)]() { publish(event); });
    }

    // Delivers queued events in order. Events enqueued by handlers during the
    // flush are delivered in the next flush.
    void flush() {
        std::vector<std::function<void()>> pending;
        pending.swap(queue_);
        for (auto& deliver : pending) {
            deliver();
        }
    }

    void clear() {
        lists_.clear();
        queue_.clear();
    }

private:
    template <typename E>
    struct Entry {
        SubscriptionId id;
        std::function<void(const E&)> fn;
        bool alive;
    };

    struct HandlerListBase {
        virtual ~HandlerListBase() = default;
        virtual bool remove(SubscriptionId id) = 0;
    };

    template <typename E>
    struct HandlerList : HandlerListBase {
        std::vector<Entry<E>> handlers;
        int depth = 0;

        bool remove(SubscriptionId id) override {
            for (auto& h : handlers) {
                if (h.id == id && h.alive) {
                    h.alive = false;
                    if (depth == 0) {
                        compact();
                    }
                    return true;
                }
            }
            return false;
        }

        void compact() {
            handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
                                          [](const Entry<E>& h) { return !h.alive; }),
                           handlers.end());
        }
    };

    template <typename E>
    HandlerList<E>& list() {
        auto& slot = lists_[std::type_index(typeid(E))];
        if (!slot) {
            slot = std::make_unique<HandlerList<E>>();
        }
        return static_cast<HandlerList<E>&>(*slot);
    }

    std::unordered_map<std::type_index, std::unique_ptr<HandlerListBase>> lists_;
    std::vector<std::function<void()>> queue_;
    SubscriptionId next_id_ = 1;
};

}  // namespace engine
