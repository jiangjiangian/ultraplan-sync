#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "EventBus.h"

TEST_CASE("EventBus delivers events to subscribers") {
    EventBus::Instance().Clear();
    int hits = 0;
    std::string captured;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event& e) { hits++; captured = e.text; });

    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "hello" });

    CHECK(hits == 1);
    CHECK(captured == "hello");
}

TEST_CASE("EventBus does not deliver to wrong type") {
    EventBus::Instance().Clear();
    int hits = 0;
    EventBus::Instance().Subscribe(EventType::KarmaChanged,
        [&](const Event&) { hits++; });
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "" });
    CHECK(hits == 0);
}

// EventBus reentrancy regression: a handler that mutates the same event
// type's subscription list during Publish must not invalidate the live
// iteration. Snapshotting the handler list before dispatch is the fix.
TEST_CASE("EventBus tolerates Subscribe + Clear from inside a handler") {
    EventBus::Instance().Clear();
    int outer_hits = 0;
    int reentrant_hits = 0;

    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event&) {
            outer_hits++;
            // mutate the same type's handler list mid-dispatch
            EventBus::Instance().Subscribe(EventType::ShowMessage,
                [&](const Event&) { reentrant_hits++; });
            EventBus::Instance().Clear();
        });

    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "" });

    CHECK(outer_hits == 1);          // outer handler ran exactly once
    CHECK(reentrant_hits == 0);      // newly-added handler must NOT run this dispatch
}
