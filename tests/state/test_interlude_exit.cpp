#include "doctest/doctest.h"
#include "engine/events/EventBus.h"
#include "quest/Flags.h"
#include "state/InterludeExit.h"
#include "quest/ChapterGate.h"
#include "state/SemesterStateMachine.h"
#include "dialog/DialogState.h"
#include "entities/Player.h"
#include "engine/math/Vec2.h"

using nccu::SemesterStateMachine;
using nccu::SemesterState;
using nccu::gfx::Vec2;

// S5b-2: the Interlude exit is the GDD-sanctioned south 觸發區 (decision
// F.1-board=C), not a board NPC. These pin the pure geometry and the
// anti-softlock invariant, then exercise the same flag->transition the
// GameController per-frame trigger drives.

TEST_CASE("InterludeExit: south-band points are inside, north is outside") {
    // A point well into the south band.
    CHECK(nccu::InInterludeExitZone(Vec2{500.0f, 2000.0f}));
    CHECK(nccu::InInterludeExitZone(Vec2{1800.0f, 1905.0f}));
    // Just north of the band edge -> outside.
    CHECK_FALSE(nccu::InInterludeExitZone(Vec2{500.0f, 1899.0f}));
    // Far north (umbrella strip, where Ch1 hands off to the Interlude).
    CHECK_FALSE(nccu::InInterludeExitZone(Vec2{500.0f, 1280.0f}));
    // Outside the x corridor.
    CHECK_FALSE(nccu::InInterludeExitZone(Vec2{50.0f, 2000.0f}));
}

TEST_CASE("InterludeExit: the market entry point is NOT in the exit zone") {
    // Anti-softlock invariant: arriving at the market must not place the
    // player already inside the exit band (that would bounce them
    // straight back out, skipping the market entirely).
    CHECK_FALSE(nccu::InInterludeExitZone(nccu::kInterludeEntry));
}

TEST_CASE("InterludeExit: entering the south zone routes to returnTo") {
    SemesterStateMachine m;
    Player p{Vec2{0, 0}};
    nccu::DialogState d;

    // Mimic the spine: in the Interlude, returning to Ch3 next.
    m.SetInterludeReturnTo(SemesterState::Chapter3_SportsDay);
    m.Transition(SemesterState::Interlude_Market);

    // Player is at the entry point — not in the zone, no exit.
    CHECK_FALSE(nccu::InInterludeExitZone(nccu::kInterludeEntry));
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);

    // Player walks into the south band: the GameController arms the flag,
    // CheckChapterGates consumes it and routes to returnTo.
    REQUIRE(nccu::InInterludeExitZone(Vec2{500.0f, 2000.0f}));
    p.SetFlag(nccu::kFlagLeaveInterlude);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter3_SportsDay);
    CHECK_FALSE(p.HasFlag(nccu::kFlagLeaveInterlude));   // consumed
}
