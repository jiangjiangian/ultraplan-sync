#include "doctest/doctest.h"
#include "EndingGate.h"
#include "SemesterStateMachine.h"
#include "Player.h"
#include "gfx/Vec2.h"
using nccu::SemesterStateMachine;
using nccu::SemesterState;

TEST_CASE("ending gate: Flag_BoughtUglyUmbrella in Ch1 -> Ending C") {
    SemesterStateMachine m; Player p{nccu::gfx::Vec2{0, 0}};
    p.SetFlag("Flag_BoughtUglyUmbrella");
    nccu::CheckEndingGates(p, m);
    CHECK(m.Current() == SemesterState::Ending_C);
}
TEST_CASE("ending gate: no flag -> stays in Ch1") {
    SemesterStateMachine m; Player p{nccu::gfx::Vec2{0, 0}};
    nccu::CheckEndingGates(p, m);
    CHECK(m.Current() == SemesterState::Chapter1_AddDrop);
}
TEST_CASE("ending gate: flag set but not in Ch1 -> no transition") {
    SemesterStateMachine m; Player p{nccu::gfx::Vec2{0, 0}};
    m.Transition(SemesterState::Interlude_Market);
    p.SetFlag("Flag_BoughtUglyUmbrella");
    nccu::CheckEndingGates(p, m);
    CHECK(m.Current() == SemesterState::Interlude_Market);
}
