#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "EventBus.h"

TEST_CASE("EventBus delivers events to subscribers") {
    EventBus::Instance().Clear();
    int hits = 0;
    std::string captured;
    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [&](const Event& e) { hits++; captured = e.text; });

    EventBus::Instance().Publish(Event{EventType::ShowMessage, {0,0}, BLACK, "hello"});

    CHECK(hits == 1);
    CHECK(captured == "hello");
}

TEST_CASE("EventBus does not deliver to wrong type") {
    EventBus::Instance().Clear();
    int hits = 0;
    EventBus::Instance().Subscribe(EventType::KarmaChanged,
        [&](const Event&) { hits++; });
    EventBus::Instance().Publish(Event{EventType::ShowMessage, {0,0}, BLACK, ""});
    CHECK(hits == 0);
}
