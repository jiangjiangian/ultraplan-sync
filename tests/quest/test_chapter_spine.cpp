#include "doctest/doctest.h"
#include "quest/Flags.h"
#include "quest/ChapterGate.h"
#include "state/EndingGate.h"
#include "controller/EventWiring.h"
#include "state/SemesterStateMachine.h"
#include "engine/events/EventBus.h"
#include "dialog/DialogState.h"
#include "entities/Player.h"
#include "engine/math/Vec2.h"
#include <string>
using nccu::SemesterStateMachine;
using nccu::SemesterState;

// Full re-entrant traversal of the progression spine:
//   Ch1 -> 市 -> Ch2 -> 市 -> Ch3 -> 市 -> Ch4
// The Interlude is entered 3x; returnTo (stored on the machine, not the
// recreated InterludeMarket object) routes each exit to the next chapter.
TEST_CASE("chapter spine: Ch1 -> 市 -> Ch2 -> 市 -> Ch3 -> 市 -> Ch4") {
    EventBus::Instance().Clear();
    SemesterStateMachine m;
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    std::string name;
    nccu::WireStateTransitionSubscribers(EventBus::Instance(), m, name);

    REQUIRE(m.Current() == SemesterState::Chapter1_AddDrop);

    // --- Ch1 -> Interlude via the existing UmbrellaClaimed path. The
    // subscriber also seeds returnTo = Ch2 (first market follows Ch1). ---
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "TrueUmbrella" });
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter2_Midterms);

    // --- Market 1 -> Ch2. Exit flag is consumed on transition. ---
    p.SetFlag(nccu::kFlagLeaveInterlude);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag(nccu::kFlagLeaveInterlude));   // consumed

    // --- Ch2 cleared -> Interlude, now returning to Ch3. ---
    p.SetFlag(nccu::kFlagCh2Cleared);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter3_SportsDay);

    // --- Market 2 -> Ch3. ---
    p.SetFlag(nccu::kFlagLeaveInterlude);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter3_SportsDay);
    CHECK_FALSE(p.HasFlag(nccu::kFlagLeaveInterlude));

    // --- Ch3 cleared -> Interlude, now returning to Ch4. ---
    p.SetFlag(nccu::kFlagCh3Cleared);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter4_Finals);

    // --- Market 3 -> Ch4 (no 4th market; Ch4 -> endings is later). ---
    p.SetFlag(nccu::kFlagLeaveInterlude);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);
    CHECK_FALSE(p.HasFlag(nccu::kFlagLeaveInterlude));

    // --- Sanity: Ch4 has no sibling-if here; the spine ends. ---
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);

    EventBus::Instance().Clear();
}

TEST_CASE("chapter spine: re-entry does not instantly exit (flag consumed)") {
    SemesterStateMachine m;
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;

    m.SetInterludeReturnTo(SemesterState::Chapter3_SportsDay);
    m.Transition(SemesterState::Interlude_Market);

    // First leave consumes Flag_LeaveInterlude.
    p.SetFlag(nccu::kFlagLeaveInterlude);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter3_SportsDay);

    // Re-enter the Interlude; without a fresh flag it must NOT exit.
    m.Transition(SemesterState::Interlude_Market);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
}

TEST_CASE("chapter spine: gate closes a still-active dialog on transition") {
    SemesterStateMachine m;
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;

    m.Transition(SemesterState::Chapter2_Midterms);
    d.Open({"queued line"});
    REQUIRE(d.Active());
    p.SetFlag(nccu::kFlagCh2Cleared);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK_FALSE(d.Active());
}

TEST_CASE("chapter spine: no flags -> no transition at any chapter") {
    SemesterStateMachine m;
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;

    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);                 // Ch1: no sibling-if
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);

    m.Transition(SemesterState::Chapter2_Midterms);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Chapter2_Midterms);

    m.Transition(SemesterState::Interlude_Market);
    nccu::CheckChapterGates(EventBus::Instance(), p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
}
