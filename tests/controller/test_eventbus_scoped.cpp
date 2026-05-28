// RAII / scoped-unsubscribe regression for EventBus (BUGLEDGER H1).
//
// EventBus::ScopedSubscribe(...) returns a movable, non-copyable
// Subscription whose destructor removes EXACTLY its handler. This proves:
//   (a) a Subscription going out of scope unsubscribes — later publishes
//       no longer call it;
//   (b) the B1/B2 dangling-capture use-after-free is prevented when the
//       handler is owned by a scoped Subscription (the capture and the
//       subscription die together, so the bus never invokes a handler
//       over a destroyed capture);
//   (c) move semantics transfer ownership without double-unsubscribe.
//
// The eventbus_isolation listener Clears the bus at every test/subcase
// boundary, so each case below starts from a clean bus.

#include "doctest/doctest.h"
#include "engine/events/EventBus.h"

#include <memory>
#include <string>

namespace {

Event Msg(std::string text) {
    return Event{EventType::ShowMessage, std::move(text)};
}

} // namespace

TEST_CASE("Subscription out of scope unsubscribes (no later delivery)") {
    int hits = 0;

    {
        auto sub = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage, [&](const Event&) { ++hits; });
        CHECK(sub.Active());

        EventBus::Instance().Publish(Msg("in-scope"));
        CHECK(hits == 1);   // delivered while the token is alive
    }   // sub dtor -> exactly this handler removed

    EventBus::Instance().Publish(Msg("after-scope"));
    CHECK(hits == 1);       // NOT called again — proves the unsubscribe
}

TEST_CASE("Scoped Subscription prevents the B1/B2 dangling-capture UAF") {
    // Reconstruct the B1/B2 footgun: a handler captures caller-owned
    // state by reference; that state then dies. With the OLD raw
    // Subscribe + manual-Clear model, forgetting Clear() before the
    // capture dies => use-after-free on the next Publish. Holding the
    // handler in a scoped Subscription destroys the subscription in
    // lock-step with the captured state, so the bus can never invoke a
    // handler over freed memory.
    int observed = 0;

    {
        // Heap-owned "caller state" so a real UAF (not just out-of-scope)
        // would be detectable by sanitizers if the unsubscribe failed.
        auto captured = std::make_unique<int>(0);

        EventBus::Subscription sub = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage,
            [ptr = captured.get(), &observed](const Event&) {
                *ptr += 1;          // dereferences the captured state
                observed = *ptr;
            });

        EventBus::Instance().Publish(Msg("alive"));
        CHECK(observed == 1);       // safe: capture + sub both alive

        // Destroy the subscription FIRST (RAII: it owns the handler that
        // captured `ptr`), THEN free the captured state. Order here mirrors
        // the safe teardown the RAII handle guarantees automatically.
        sub.Reset();
        CHECK_FALSE(sub.Active());
        captured.reset();           // backing memory freed
    }

    // Publishing after the captured state is gone must NOT touch it.
    EventBus::Instance().Publish(Msg("dangling"));
    CHECK(observed == 1);           // handler never ran again -> no UAF
}

TEST_CASE("Subscription move transfers ownership, no double-unsubscribe") {
    int hits = 0;

    SUBCASE("move-construct") {
        EventBus::Subscription a = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage, [&](const Event&) { ++hits; });

        EventBus::Subscription b(std::move(a));
        CHECK_FALSE(a.Active());     // moved-from owns nothing
        CHECK(b.Active());

        EventBus::Instance().Publish(Msg("x"));
        CHECK(hits == 1);            // exactly one live handler

        // a's destruction must be a no-op (no double-unsubscribe / no
        // removal of b's handler). b still owns the live subscription.
    }

    SUBCASE("move-assign over a live subscription") {
        int otherHits = 0;
        EventBus::Subscription a = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage, [&](const Event&) { ++hits; });
        EventBus::Subscription b = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage, [&](const Event&) { ++otherHits; });

        // Move-assign must Reset b's old handler (otherHits frozen) then
        // take over a's, leaving exactly one live handler (hits).
        b = std::move(a);
        CHECK_FALSE(a.Active());
        CHECK(b.Active());

        EventBus::Instance().Publish(Msg("y"));
        CHECK(hits == 1);            // a's handler, now owned by b
        CHECK(otherHits == 0);       // b's old handler was unsubscribed
    }

    SUBCASE("destroying both copies removes the handler exactly once") {
        {
            EventBus::Subscription a = EventBus::Instance().ScopedSubscribe(
                EventType::ShowMessage, [&](const Event&) { ++hits; });
            EventBus::Subscription b(std::move(a));
            EventBus::Instance().Publish(Msg("z1"));
            CHECK(hits == 1);
        }   // both a (no-op) and b (real) destruct here
        EventBus::Instance().Publish(Msg("z2"));
        CHECK(hits == 1);            // gone after the single owner died
    }
}

TEST_CASE("ScopedSubscribe coexists with raw Subscribe / Clear") {
    int rawHits    = 0;
    int scopedHits = 0;

    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event&) { ++rawHits; });

    {
        auto sub = EventBus::Instance().ScopedSubscribe(
            EventType::ShowMessage, [&](const Event&) { ++scopedHits; });
        EventBus::Instance().Publish(Msg("a"));
        CHECK(rawHits == 1);
        CHECK(scopedHits == 1);
    }   // scoped handler removed; raw handler untouched

    EventBus::Instance().Publish(Msg("b"));
    CHECK(rawHits == 2);             // raw subscriber still alive
    CHECK(scopedHits == 1);          // scoped one gone

    // Clear() still nukes everything (backward-compatible), and a
    // Subscription whose handler Clear() already removed destructs
    // harmlessly (Unsubscribe is a no-op when the id is gone).
    auto sub = EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage, [&](const Event&) { ++scopedHits; });
    EventBus::Instance().Clear();
    EventBus::Instance().Publish(Msg("c"));
    CHECK(rawHits == 2);
    CHECK(scopedHits == 1);
}   // sub dtor here: Unsubscribe(id) is a safe no-op post-Clear
