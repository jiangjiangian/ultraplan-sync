// Cycle 9.G — Two HUD channels (Plan B) regression suite.
//
// Pre-9.G, World held a single hudMessage_ slot; the 9.A.2 chapter-
// clear toast at every Ch->IL transition was visible for 0.02 s (1
// frame) because the IL arrival hint published right after and
// overwrote the same slot (cycle9f-post-iteration-diagnosis §B). Plan A
// (9.B publish-order swap) only addressed the umbrella-vs-chapter race;
// the arrival-hint clobber stayed live until Plan B split the channel
// in two. These tests pin the post-split invariants:
//
//   1. A Top-slot publish lands only on Top; the Bottom slot stays
//      empty. (And vice versa.)
//   2. Same-frame Top + Bottom publishes coexist — neither overwrites
//      the other.
//   3. The Ch->IL transition pattern (chapter toast Top + arrival hint
//      Bottom on adjacent publishes) leaves BOTH lines visible.
//   4. Top expiry doesn't leak text into Bottom.
//   5. DismissHud(slot) only kills the slot it targets; the default
//      DismissHud() kills both.
//
// The eventbus_isolation listener clears the bus at the case boundary
// so each test starts clean.

#include "doctest/doctest.h"
#include "ChapterToast.h"
#include "EventBus.h"
#include "EventWiring.h"
#include "HudSlot.h"
#include "MessageView.h"   // kHudTtl
#include "Player.h"
#include "SemesterState.h"
#include "SemesterStateMachine.h"
#include "World.h"
#include "gfx/Vec2.h"

using nccu::HudSlot;
using nccu::SemesterState;
using nccu::World;
using nccu::gfx::Vec2;

TEST_CASE("World::SetHudMessage routes by slot, no cross-channel leak") {
    World w("", /*loadSprites=*/false);

    // Fresh world: both slots empty, neither expired.
    REQUIRE(w.HudMessage(HudSlot::Top).empty());
    REQUIRE(w.HudMessage(HudSlot::Bottom).empty());
    REQUIRE_FALSE(w.HudExpired(HudSlot::Top));
    REQUIRE_FALSE(w.HudExpired(HudSlot::Bottom));

    SUBCASE("Top write does not touch Bottom") {
        w.SetHudMessage(HudSlot::Top, "✓ 章節清關 — 進入幕間市集");
        CHECK(w.HudMessage(HudSlot::Top) == "✓ 章節清關 — 進入幕間市集");
        CHECK(w.HudMessage(HudSlot::Bottom).empty());
        CHECK(w.HudAge(HudSlot::Top) == doctest::Approx(0.0f));
    }
    SUBCASE("Bottom write does not touch Top") {
        w.SetHudMessage(HudSlot::Bottom, "你撿到了 TrueUmbrella，雨停了。");
        CHECK(w.HudMessage(HudSlot::Bottom)
              == "你撿到了 TrueUmbrella，雨停了。");
        CHECK(w.HudMessage(HudSlot::Top).empty());
        CHECK(w.HudAge(HudSlot::Bottom) == doctest::Approx(0.0f));
    }
    SUBCASE("Backward-compat SetHudMessage(text) writes to Bottom") {
        // Pre-9.G call sites (tests, ad-hoc probes) keep landing on the
        // Bottom slot exactly as they did before the split.
        w.SetHudMessage("legacy");
        CHECK(w.HudMessage(HudSlot::Bottom) == "legacy");
        CHECK(w.HudMessage(HudSlot::Top).empty());
        // The default HudMessage() / HudAge() accessors also reference
        // Bottom — same backward-compat property at the read surface.
        CHECK(w.HudMessage() == "legacy");
        CHECK(w.HudAge() == doctest::Approx(0.0f));
    }
}

TEST_CASE("Same-frame Top + Bottom publishes coexist (no clobber)") {
    // The key Plan B contract: a chapter-clear (Top) and an arrival hint
    // (Bottom) publishing on the same frame must both end up visible.
    // The end state of World.HudMessage(Top) AND HudMessage(Bottom) is
    // the assertion — pre-9.G the second publish overwrote the first.
    //
    // NOTE: the eventbus_isolation listener clears the bus at every
    // SUBCASE boundary, so the World + WireHudMessageSubscriber MUST
    // live INSIDE each SUBCASE (same idiom as the other transition /
    // karma tests that capture a probe by reference).

    SUBCASE("Top then Bottom") {
        World w("", /*loadSprites=*/false);
        nccu::WireHudMessageSubscriber(EventBus::Instance(), w);

        EventBus::Instance().Publish(Event{
            EventType::ShowMessage, "✓ 章節清關 — 進入幕間市集",
            HudSlot::Top});
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage, "市集中央。逛完後往南離開",
            HudSlot::Bottom});

        CHECK(w.HudMessage(HudSlot::Top) == "✓ 章節清關 — 進入幕間市集");
        CHECK(w.HudMessage(HudSlot::Bottom) == "市集中央。逛完後往南離開");
    }

    SUBCASE("Bottom then Top — order does not matter") {
        World w("", /*loadSprites=*/false);
        nccu::WireHudMessageSubscriber(EventBus::Instance(), w);

        EventBus::Instance().Publish(Event{
            EventType::ShowMessage, "市集中央。逛完後往南離開",
            HudSlot::Bottom});
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage, "✓ 章節清關 — 進入幕間市集",
            HudSlot::Top});

        CHECK(w.HudMessage(HudSlot::Top) == "✓ 章節清關 — 進入幕間市集");
        CHECK(w.HudMessage(HudSlot::Bottom) == "市集中央。逛完後往南離開");
    }
}

TEST_CASE("Ch->IL transition pattern: both lines survive into next frame") {
    // Simulates exactly the production sequence (GameController.cpp:154-
    // 163 + EventWiring's UmbrellaClaimed handler):
    //
    //   1. Chapter-clear toast publishes (Top)       — frame N
    //   2. Roster respawn -> IL arrival hint
    //      publishes (Bottom)                        — frame N (or N+1)
    //   3. TickHud advances both ages by one 60-fps step.
    //
    // After step 3 the player still sees BOTH lines (both ages well
    // under kHudTtl). Pre-9.G, step 2 erased step 1's text — the bug
    // this whole channel split exists to kill.
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    nccu::WireHudMessageSubscriber(EventBus::Instance(), w);

    nccu::PublishChapterTransitionToast(SemesterState::Interlude_Market);
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage, nccu::kInterludeArrivalHint,
        HudSlot::Bottom});
    w.TickHud(1.0f / 60.0f);

    CHECK(w.HudMessage(HudSlot::Top) == "✓ 章節清關 — 進入幕間市集");
    CHECK(w.HudMessage(HudSlot::Bottom) == nccu::kInterludeArrivalHint);
    CHECK_FALSE(w.HudExpired(HudSlot::Top));
    CHECK_FALSE(w.HudExpired(HudSlot::Bottom));
    // Both ages stepped by the same dt — neither is stale.
    CHECK(w.HudAge(HudSlot::Top) == doctest::Approx(1.0f / 60.0f));
    CHECK(w.HudAge(HudSlot::Bottom) == doctest::Approx(1.0f / 60.0f));

    EventBus::Instance().Clear();
}

TEST_CASE("Top channel expiry never leaks text into Bottom") {
    // After the Top toast ages past kHudTtl, a Bottom publish must NOT
    // somehow inherit the expired text — slots are truly independent.
    World w("", /*loadSprites=*/false);

    w.SetHudMessage(HudSlot::Top, "expiring");
    // Age past TTL.
    w.TickHud(nccu::kHudTtl + 0.5f);
    REQUIRE(w.HudExpired(HudSlot::Top));
    REQUIRE_FALSE(w.HudExpired(HudSlot::Bottom));  // empty, not expired

    // Bottom write lands cleanly.
    w.SetHudMessage(HudSlot::Bottom, "fresh");
    CHECK(w.HudMessage(HudSlot::Bottom) == "fresh");
    CHECK(w.HudAge(HudSlot::Bottom) == doctest::Approx(0.0f));
    // Top is still expired-but-held (the View's fade-out contract from
    // 9.B L9 keeps the buffer around — HudExpired flags it for the
    // harness to suppress, no auto-clear).
    CHECK(w.HudMessage(HudSlot::Top) == "expiring");
    CHECK(w.HudExpired(HudSlot::Top));
}

TEST_CASE("DismissHud per-slot vs default both-slot semantics") {
    World w("", /*loadSprites=*/false);
    w.SetHudMessage(HudSlot::Top,    "top");
    w.SetHudMessage(HudSlot::Bottom, "bottom");
    REQUIRE_FALSE(w.HudExpired(HudSlot::Top));
    REQUIRE_FALSE(w.HudExpired(HudSlot::Bottom));

    SUBCASE("DismissHud(Top) only kills Top") {
        w.DismissHud(HudSlot::Top);
        CHECK(w.HudExpired(HudSlot::Top));
        CHECK_FALSE(w.HudExpired(HudSlot::Bottom));
    }
    SUBCASE("DismissHud(Bottom) only kills Bottom") {
        w.DismissHud(HudSlot::Bottom);
        CHECK_FALSE(w.HudExpired(HudSlot::Top));
        CHECK(w.HudExpired(HudSlot::Bottom));
    }
    SUBCASE("DismissHud() kills both slots") {
        // SC 2.2.2 (skip-toast) input still works in one keystroke
        // regardless of how many channels are live.
        w.DismissHud();
        CHECK(w.HudExpired(HudSlot::Top));
        CHECK(w.HudExpired(HudSlot::Bottom));
    }
}

TEST_CASE("Default-slot Event lands on Bottom (backward compat)") {
    // A pre-9.G publisher that builds Event{type, text} without naming
    // a slot must keep landing on Bottom — every existing call site
    // (ProfessorTrapUmbrella, CashPickup, NPC dialog, Vendor, …) is one
    // of these. Verified through the live HUD subscriber wiring.
    EventBus::Instance().Clear();
    World w("", /*loadSprites=*/false);
    nccu::WireHudMessageSubscriber(EventBus::Instance(), w);

    EventBus::Instance().Publish(Event{EventType::ShowMessage, "legacy"});
    CHECK(w.HudMessage(HudSlot::Bottom) == "legacy");
    CHECK(w.HudMessage(HudSlot::Top).empty());

    EventBus::Instance().Clear();
}

TEST_CASE("Chapter toast publish carries HudSlot::Top in the Event") {
    // Capture every published ShowMessage and inspect the slot field —
    // this is the publish-side contract that ChapterToast.h opted the
    // chapter / ending transitions into Plan B.
    EventBus::Instance().Clear();
    HudSlot lastSlot = HudSlot::Bottom;
    std::string lastText;
    auto sub = EventBus::Instance().ScopedSubscribe(
        EventType::ShowMessage,
        [&](const Event& e) { lastSlot = e.slot; lastText = e.text; });

    nccu::PublishChapterTransitionToast(SemesterState::Interlude_Market);
    CHECK(lastSlot == HudSlot::Top);
    CHECK(lastText == "✓ 章節清關 — 進入幕間市集");

    nccu::PublishChapterTransitionToast(SemesterState::Ending_A);
    CHECK(lastSlot == HudSlot::Top);
    CHECK(lastText == "✓ 抵達結局");

    EventBus::Instance().Clear();
}
