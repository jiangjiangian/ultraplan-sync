#ifndef EVENT_SINK_H_
#define EVENT_SINK_H_
#include "engine/events/EventBus.h"

namespace nccu::events {

// Plan P2 step 3 — entity-layer publish seam. Mirrors the deliberately-
// retained gfx::Input::SetSource / gfx::Time::SetFixedStep harness seams
// blueprint phase 2 calls out as "the deliberate, documented mechanism"
// to keep. The entity layer (umbrella family / consumables / NPC /
// pickups / Vendor / BuildingTracker) historically called
// EventBus::Instance().Publish(...) directly, threading an EventBus&
// through every Interact/Consume/OnPickup body would force a cascade
// across IInteractable's role signature and every test fixture. Instead
// entities publish through this seam:
//
//     nccu::events::Sink().Publish(Event{...});
//
// main.cpp / GameController call `SetSink(&bus)` once at startup so the
// per-process current sink is the SAME EventBus the controller threads
// through the chapter-transition stack (step 1) and the quest hooks
// (step 2). A test can SetSink to a local bus and back to Instance()
// for isolation, or leave it null (the default) so the seam falls
// through to EventBus::Instance() — keeping every legacy callsite
// byte-identical until explicitly retargeted.
//
// Net effect: entity .cpp files no longer name EventBus::Instance() at
// any publish site; they name the SEAM, which the controller / tests
// own.
void SetSink(EventBus* bus) noexcept;

[[nodiscard]] EventBus& Sink() noexcept;

} // namespace nccu::events

#endif // EVENT_SINK_H_
