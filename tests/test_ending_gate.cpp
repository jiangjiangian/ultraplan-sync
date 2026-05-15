#include "doctest/doctest.h"
#include "EndingGate.h"
#include "SemesterStateMachine.h"
#include "DialogState.h"
#include "Player.h"
#include "gfx/Vec2.h"
using nccu::SemesterStateMachine;
using nccu::SemesterState;

TEST_CASE("ending gate: Flag_BoughtUglyUmbrella in Ch1 -> Ending C") {
    SemesterStateMachine m; Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    p.SetFlag("Flag_BoughtUglyUmbrella");
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_C);
}
TEST_CASE("ending gate: no flag -> stays in Ch1") {
    SemesterStateMachine m; Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);
}
TEST_CASE("ending gate: flag set but not in Ch1 -> no transition") {
    SemesterStateMachine m; Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    m.Transition(SemesterState::Interlude_Market);
    p.SetFlag("Flag_BoughtUglyUmbrella");
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
}

TEST_CASE("ending gate closes a still-active dialog on transition") {
    SemesterStateMachine m; Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    d.Open({"queued consequence line"});      // simulate the buy-choice nextLines
    REQUIRE(d.Active());
    p.SetFlag("Flag_BoughtUglyUmbrella");
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_C);
    CHECK_FALSE(d.Active());                   // terminal screen: no stale dialog
}

TEST_CASE("ending gate: no-op once Ch1 already advanced (TrueUmbrella race)") {
    // 1c's TrueUmbrella gate fired first -> Interlude. A later buy must
    // NOT bounce the player into Ending C.
    SemesterStateMachine m; Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    m.Transition(SemesterState::Interlude_Market);
    p.SetFlag("Flag_BoughtUglyUmbrella");
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Interlude_Market);
}
