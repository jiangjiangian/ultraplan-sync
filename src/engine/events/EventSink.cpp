#include "engine/events/EventSink.h"

namespace nccu::events {

namespace {
// Per-process pointer to the EventBus entity callsites should publish
// through. Null by default so a no-setup translation unit (a doctest
// that only constructs a Player) falls through to EventBus::Instance()
// and behaves byte-identically to the pre-P2 path. main.cpp sets it
// alongside the GameController; the controller's dtor / main's outer
// teardown reset it to null so a Restart cycle re-binds cleanly.
EventBus* g_sink = nullptr;
} // namespace

void SetSink(EventBus* bus) noexcept { g_sink = bus; }

EventBus& Sink() noexcept {
    return g_sink ? *g_sink : EventBus::Instance();
}

} // namespace nccu::events
