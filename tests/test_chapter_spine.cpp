#include "doctest/doctest.h"
#include "ChapterGate.h"
#include "EndingGate.h"
#include "EventWiring.h"
#include "SemesterStateMachine.h"
#include "EventBus.h"
#include "DialogState.h"
#include "Player.h"
#include "gfx/Vec2.h"
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
    p.SetFlag("Flag_LeaveInterlude");
    nccu::CheckChapterGates(p, m, d);
    CHECK(m.Current() == SemesterState::Chapter2_Midterms);
    CHECK_FALSE(p.HasFlag("Flag_LeaveInterlude"));   // consumed

    // --- Ch2 cleared -> Interlude, now returning to Ch3. ---
    p.SetFlag("Flag_Ch2Cleared");
    nccu::CheckChapterGates(p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter3_SportsDay);

    // --- Market 2 -> Ch3. ---
    p.SetFlag("Flag_LeaveInterlude");
    nccu::CheckChapterGates(p, m, d);
    CHECK(m.Current() == SemesterState::Chapter3_SportsDay);
    CHECK_FALSE(p.HasFlag("Flag_LeaveInterlude"));

    // --- Ch3 cleared -> Interlude, now returning to Ch4. ---
    p.SetFlag("Flag_Ch3Cleared");
    nccu::CheckChapterGates(p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK(m.InterludeReturnTo() == SemesterState::Chapter4_Finals);

    // --- Market 3 -> Ch4 (no 4th market; Ch4 -> endings is later). ---
    p.SetFlag("Flag_LeaveInterlude");
    nccu::CheckChapterGates(p, m, d);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);
    CHECK_FALSE(p.HasFlag("Flag_LeaveInterlude"));

    // --- Sanity: Ch4 has no sibling-if here; the spine ends. ---
    nccu::CheckChapterGates(p, m, d);
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
    p.SetFlag("Flag_LeaveInterlude");
    nccu::CheckChapterGates(p, m, d);
    CHECK(m.Current() == SemesterState::Chapter3_SportsDay);

    // Re-enter the Interlude; without a fresh flag it must NOT exit.
    m.Transition(SemesterState::Interlude_Market);
    nccu::CheckChapterGates(p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
}

TEST_CASE("chapter spine: gate closes a still-active dialog on transition") {
    SemesterStateMachine m;
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;

    m.Transition(SemesterState::Chapter2_Midterms);
    d.Open({"queued line"});
    REQUIRE(d.Active());
    p.SetFlag("Flag_Ch2Cleared");
    nccu::CheckChapterGates(p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
    CHECK_FALSE(d.Active());
}

TEST_CASE("chapter spine: no flags -> no transition at any chapter") {
    SemesterStateMachine m;
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;

    nccu::CheckChapterGates(p, m, d);                 // Ch1: no sibling-if
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);

    m.Transition(SemesterState::Chapter2_Midterms);
    nccu::CheckChapterGates(p, m, d);
    CHECK(m.Current() == SemesterState::Chapter2_Midterms);

    m.Transition(SemesterState::Interlude_Market);
    nccu::CheckChapterGates(p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
}
