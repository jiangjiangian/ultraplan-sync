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

// Regression — the Ch4 助教 (d) finale soft-lock. Found by a live
// playtest (/tmp/probe_ta_hard.txt = the full honest A-spine but the
// 助教 finale picks 「質問／強硬索回」 instead of 體諒): the player
// completed every chapter, reached Ch4, made the finale choice — which
// sets Flag_TaFinaleChoiceMade and PERMANENTLY locks the menu
// (DialogOpener recap, no re-choose) — but qualified for none of A/B/C,
// so CheckEndingGates fell through and the game was stuck in Ch4
// forever (8071 frames, 0 frames in 結局). An unwinnable state is a
// CLAUDE.md §5 red-line violation. Fix: once the finale choice is made,
// the gate is TOTAL — the cold choice → B (thematically 屠龍者終成惡龍),
// 體諒-but-not-perfect → C (the GDD-designated 破財消災 Normal default).
TEST_CASE("ending gate: 助教 finale is TOTAL — no fall-through soft-lock") {
    SUBCASE("cold finale (質問/強硬, honest spine) -> Ending B") {
        // The exact /tmp/probe_ta_hard.txt end-state: full honest run
        // (high karma, TrueUmbrella reclaimed) but chose NOT to console
        // the TA. Pre-fix this fell through every gate.
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100);                          // honest, karma 95-100
        p.SetFlag("Flag_HasTrueUmbrella");        // reclaimed in Ch4
        p.SetFlag("Flag_TaFinaleChoiceMade");     // finale choice made
        // NO Flag_ConsoledTA (chose 質問/強硬), NO cursed, NO ugly buy.
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_B);   // was: stuck Ch4
    }
    SUBCASE("體諒 but karma<=80 (not-perfect honest) -> Ending C Normal") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;  // karma ~50
        p.SetFlag("Flag_ConsoledTA");             // chose 體諒
        p.SetFlag("Flag_TaFinaleChoiceMade");
        // karma not > 80 -> not A; not cursed/cold -> not B.
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_C);   // was: stuck Ch4
    }
    SUBCASE("體諒 + karma>80 but no TrueUmbrella -> Ending C Normal") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100);
        p.SetFlag("Flag_ConsoledTA");
        p.SetFlag("Flag_TaFinaleChoiceMade");
        // missing Flag_HasTrueUmbrella -> not A; consoled -> not cold B.
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_C);   // was: stuck Ch4
    }
    SUBCASE("precedence: cold finale never overrides an earned A") {
        // 體諒 was chosen AND fully earned -> A still wins (the finale
        // flag is set but Flag_ConsoledTA is too, so coldFinale=false).
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100);
        p.SetFlag("Flag_HasTrueUmbrella");
        p.SetFlag("Flag_ConsoledTA");
        p.SetFlag("Flag_TaFinaleChoiceMade");
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_A);
    }
}

TEST_CASE("ending gate: pre-finale Ch4 free-roam is byte-unchanged") {
    // The C-default is gated STRICTLY on Flag_TaFinaleChoiceMade so a
    // player exploring Ch4 BEFORE talking to the TA never gets shoved
    // into an ending. Partial-A and empty states with NO finale flag
    // must still stay in Ch4 (these mirror the pre-existing
    // "any one condition missing" / "no flags" guarantees, asserted
    // here against the new total-gate logic so the regression locks the
    // gating-key choice, not just the happy path).
    SUBCASE("karma>80 + TrueUmbrella, finale NOT yet made -> stays Ch4") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100); p.SetFlag("Flag_HasTrueUmbrella");
        // no Flag_ConsoledTA, no Flag_TaFinaleChoiceMade
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Chapter4_Finals);
    }
    SUBCASE("no flags, finale NOT made -> stays Ch4") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Chapter4_Finals);
    }
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
