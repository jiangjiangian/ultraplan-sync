#include "doctest/doctest.h"
#include "EndingGate.h"
#include "SemesterStateMachine.h"
#include "DialogState.h"
#include "Player.h"
#include "gfx/Vec2.h"
using nccu::SemesterStateMachine;
using nccu::SemesterState;

// S5e-2b: the three endings resolve in CheckEndingGates, ALL guarded
// to Chapter4_Finals. The old Ch1+Flag_BoughtUglyUmbrella→Ending C is
// gone (C.1: the Ch1 阿姨 buy now only seeds Flag_KnowsUglyUmbrella;
// the real buy is the Ch4 集英樓 Vendor → Flag_BoughtUglyUmbrella).
// SemesterStateMachine is non-movable, so each case constructs it
// inline (same idiom as test_chapter_spine).

TEST_CASE("ending gate: no gate fires outside Chapter4_Finals") {
    for (auto s : {SemesterState::Chapter1_AddDrop,
                   SemesterState::Interlude_Market,
                   SemesterState::Chapter2_Midterms,
                   SemesterState::Chapter3_SportsDay}) {
        SemesterStateMachine m;
        if (s != SemesterState::Chapter1_AddDrop) m.Transition(s);
        Player p{nccu::gfx::Vec2{0, 0}};
        nccu::DialogState d;
        // Every ending's flags set — none may fire before Ch4.
        p.SetFlag("Flag_HasTrueUmbrella");
        p.SetFlag("Flag_ConsoledTA");
        p.SetFlag("Flag_TookCursedUmbrella");
        p.SetFlag("Flag_BoughtUglyUmbrella");
        p.AddKarma(100);
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == s);
    }
}

TEST_CASE("ending gate A: karma>80 + TrueUmbrella + 體諒 -> Ending A") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    p.AddKarma(100);                         // > 80 (clamped 100)
    p.SetFlag("Flag_HasTrueUmbrella");
    p.SetFlag("Flag_ConsoledTA");
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_A);
}

TEST_CASE("ending gate A: any one condition missing -> not Ending A") {
    SUBCASE("no 體諒") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100); p.SetFlag("Flag_HasTrueUmbrella");
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Chapter4_Finals);
    }
    SUBCASE("no TrueUmbrella") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100); p.SetFlag("Flag_ConsoledTA");
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Chapter4_Finals);
    }
    SUBCASE("karma not > 80") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;   // default ~50
        p.SetFlag("Flag_HasTrueUmbrella"); p.SetFlag("Flag_ConsoledTA");
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Chapter4_Finals);
    }
}

TEST_CASE("ending gate B: cursed umbrella OR karma<0 -> Ending B") {
    SUBCASE("cursed") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.SetFlag("Flag_TookCursedUmbrella");
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_B);
    }
    SUBCASE("karma below zero") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(-200);                    // clamps to -100 (<0)
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_B);
    }
}

TEST_CASE("ending gate C: bought the ugly umbrella -> Ending C") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    p.SetFlag("Flag_BoughtUglyUmbrella");
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_C);
}

TEST_CASE("ending gate: C.1 — Ch1 seed flag does NOT trigger any ending") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    p.SetFlag("Flag_KnowsUglyUmbrella");     // the Ch1 阿姨 seed
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);   // 伏筆, no ending
}

TEST_CASE("ending gate: precedence A > B > C") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    // All three satisfiable at once: the honest path wins.
    p.AddKarma(100);
    p.SetFlag("Flag_HasTrueUmbrella");
    p.SetFlag("Flag_ConsoledTA");
    p.SetFlag("Flag_TookCursedUmbrella");
    p.SetFlag("Flag_BoughtUglyUmbrella");
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_A);
}

TEST_CASE("ending gate: no flags -> stays in Ch4") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);
}

TEST_CASE("ending gate closes a still-active dialog on transition") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    d.Open({"queued consequence line"});
    REQUIRE(d.Active());
    p.SetFlag("Flag_BoughtUglyUmbrella");
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_C);
    CHECK_FALSE(d.Active());                  // terminal screen: no stale dialog
}
