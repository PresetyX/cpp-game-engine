#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "engine/core/event_bus.hpp"

using engine::EventBus;
using engine::SubscriptionId;

namespace {
struct Damage {
    int amount;
};
struct Heal {
    int amount;
};
}  // namespace

TEST(EventBus, DeliversToMatchingTypeOnly) {
    EventBus bus;
    int damage = 0;
    int heal = 0;
    bus.subscribe<Damage>([&](const Damage& e) { damage += e.amount; });
    bus.subscribe<Heal>([&](const Heal& e) { heal += e.amount; });

    bus.publish(Damage{5});
    EXPECT_EQ(damage, 5);
    EXPECT_EQ(heal, 0);
}

TEST(EventBus, SubscribersRunInSubscriptionOrder) {
    EventBus bus;
    std::vector<int> order;
    bus.subscribe<Damage>([&](const Damage&) { order.push_back(1); });
    bus.subscribe<Damage>([&](const Damage&) { order.push_back(2); });
    bus.publish(Damage{1});
    EXPECT_EQ(order, (std::vector<int>{1, 2}));
}

TEST(EventBus, UnsubscribeStopsDelivery) {
    EventBus bus;
    int calls = 0;
    const SubscriptionId id = bus.subscribe<Damage>([&](const Damage&) { ++calls; });
    bus.publish(Damage{1});
    bus.unsubscribe(id);
    bus.publish(Damage{1});
    EXPECT_EQ(calls, 1);
}

TEST(EventBus, PublishWithoutSubscribersIsSafe) {
    EventBus bus;
    bus.publish(Damage{1});
    SUCCEED();
}

TEST(EventBus, UnsubscribeUnknownIdIsSafe) {
    EventBus bus;
    bus.unsubscribe(12345);
    SUCCEED();
}

TEST(EventBus, HandlerCanUnsubscribeItselfDuringDispatch) {
    EventBus bus;
    int first = 0;
    int second = 0;
    SubscriptionId first_id = 0;
    first_id = bus.subscribe<Damage>([&](const Damage&) {
        ++first;
        bus.unsubscribe(first_id);
    });
    bus.subscribe<Damage>([&](const Damage&) { ++second; });

    bus.publish(Damage{1});
    bus.publish(Damage{1});
    EXPECT_EQ(first, 1);
    EXPECT_EQ(second, 2);
}

TEST(EventBus, HandlerCanUnsubscribeLaterHandlerDuringDispatch) {
    EventBus bus;
    int second = 0;
    SubscriptionId second_id = 0;
    bus.subscribe<Damage>([&](const Damage&) { bus.unsubscribe(second_id); });
    second_id = bus.subscribe<Damage>([&](const Damage&) { ++second; });

    bus.publish(Damage{1});
    EXPECT_EQ(second, 0);
}

TEST(EventBus, SubscribeDuringDispatchTakesEffectNextPublish) {
    EventBus bus;
    int late = 0;
    bool added = false;
    bus.subscribe<Damage>([&](const Damage&) {
        if (!added) {
            added = true;
            bus.subscribe<Damage>([&](const Damage&) { ++late; });
        }
    });

    bus.publish(Damage{1});
    EXPECT_EQ(late, 0);
    bus.publish(Damage{1});
    EXPECT_EQ(late, 1);
}

TEST(EventBus, EnqueueDeliversOnlyOnFlush) {
    EventBus bus;
    std::vector<int> seen;
    bus.subscribe<Damage>([&](const Damage& e) { seen.push_back(e.amount); });

    bus.enqueue(Damage{1});
    bus.enqueue(Damage{2});
    EXPECT_TRUE(seen.empty());

    bus.flush();
    EXPECT_EQ(seen, (std::vector<int>{1, 2}));

    bus.flush();  // nothing left
    EXPECT_EQ(seen.size(), 2u);
}

TEST(EventBus, EventsEnqueuedDuringFlushWaitForNextFlush) {
    EventBus bus;
    int heals = 0;
    bus.subscribe<Damage>([&](const Damage&) { bus.enqueue(Heal{1}); });
    bus.subscribe<Heal>([&](const Heal&) { ++heals; });

    bus.enqueue(Damage{1});
    bus.flush();
    EXPECT_EQ(heals, 0);
    bus.flush();
    EXPECT_EQ(heals, 1);
}

TEST(EventBus, ClearRemovesSubscribersAndQueue) {
    EventBus bus;
    int calls = 0;
    bus.subscribe<Damage>([&](const Damage&) { ++calls; });
    bus.enqueue(Damage{1});
    bus.clear();
    bus.flush();
    bus.publish(Damage{1});
    EXPECT_EQ(calls, 0);
}
