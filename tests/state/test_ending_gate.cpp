#include "doctest/doctest.h"
#include "quest/Flags.h"
#include "state/EndingGate.h"
#include "state/SemesterStateMachine.h"
#include "dialog/DialogState.h"
#include "entities/Player.h"
#include "ui/EndingView.h"   // IsEndingState (G1 TOTAL proof)
#include "gfx/Vec2.h"
using nccu::SemesterStateMachine;
using nccu::SemesterState;
using nccu::IsEndingState;

// S5e-2b: the three endings resolve in CheckEndingGates, ALL guarded
// to Chapter4_Finals. The old Ch1+Flag_BoughtUglyUmbrella→Ending C is
// gone — the Ch1 阿姨 (c) is now a pure narrative seed (cycle-8
// audit F1 removed the previous inert Flag_KnowsUglyUmbrella per the
// B3 precedent); the real Ending-C trigger is the Ch4 集英樓 Vendor →
// Flag_BoughtUglyUmbrella. SemesterStateMachine is non-movable, so
// each case constructs it inline (same idiom as test_chapter_spine).

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
        p.SetFlag(nccu::kFlagHasTrueUmbrella);
        p.SetFlag(nccu::kFlagConsoledTA);
        p.SetFlag(nccu::kFlagTookCursedUmbrella);
        p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
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
    p.SetFlag(nccu::kFlagHasTrueUmbrella);
    p.SetFlag(nccu::kFlagConsoledTA);
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_A);
}

TEST_CASE("ending gate A: any one condition missing -> not Ending A") {
    SUBCASE("no 體諒 (no Flag_ConsoledTA) -> not A, and without it, stays Ch4") {
        // karma>80 + 持真傘 but never chose 體諒: A needs all three, and with
        // NO Flag_ConsoledTA the new D gate also doesn't fire, so this state
        // (no finale made) correctly stays in Ch4 until the player decides.
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100); p.SetFlag(nccu::kFlagHasTrueUmbrella);
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Chapter4_Finals);
    }
    SUBCASE("no TrueUmbrella but 體諒+karma>80 -> Ending D (G1), not A") {
        // Was "stays Ch4"; post-G1 a 體諒 player who lacks the reclaimed
        // true umbrella lands the bittersweet D (風雨同行), NOT A.
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100); p.SetFlag(nccu::kFlagConsoledTA);
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_D);
    }
    SUBCASE("karma not > 80 but 體諒+持真傘 -> Ending D (G1), not A") {
        // Was "stays Ch4"; post-G1 體諒 with karma in [0,80] lands D.
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;   // default ~50
        p.SetFlag(nccu::kFlagHasTrueUmbrella); p.SetFlag(nccu::kFlagConsoledTA);
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_D);
    }
}

TEST_CASE("ending gate B: cursed umbrella OR karma<0 -> Ending B") {
    SUBCASE("cursed") {
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.SetFlag(nccu::kFlagTookCursedUmbrella);
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
    p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_C);
}

TEST_CASE("ending gate: Ch1 阿姨 (c) buy is a pure narrative seed (audit F1)") {
    // Pre-F1: chapter1.md (c) seeded Flag_KnowsUglyUmbrella; this test
    // pinned that the gate ignored that seed. Post-F1: the (c) substate
    // carries no flag at all (B3 precedent removal), so the Ch1 buy
    // adds nothing to the player state — only Flag_BoughtUglyUmbrella
    // (Ch4 集英樓 Vendor) triggers Ending C. The contract guarded here
    // is unchanged: Ending-C never fires on the Ch1 seed path alone.
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    // Sanity: no Ch1 flag — the seed leaves player state untouched.
    CHECK_FALSE(p.HasFlag(nccu::kFlagBoughtUglyUmbrella));
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);   // no ending
}

TEST_CASE("ending gate: precedence A > B > C") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    // All three satisfiable at once: the honest path wins.
    p.AddKarma(100);
    p.SetFlag(nccu::kFlagHasTrueUmbrella);
    p.SetFlag(nccu::kFlagConsoledTA);
    p.SetFlag(nccu::kFlagTookCursedUmbrella);
    p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
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
        p.SetFlag(nccu::kFlagHasTrueUmbrella);        // reclaimed in Ch4
        p.SetFlag(nccu::kFlagTaFinaleChoiceMade);     // finale choice made
        // NO Flag_ConsoledTA (chose 質問/強硬), NO cursed, NO ugly buy.
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_B);   // was: stuck Ch4
    }
    SUBCASE("體諒 but karma<=80 (not-perfect honest) -> Ending D (G1)") {
        // G1 reslotted this from C to the bittersweet D — 體諒 with karma in
        // [0,80] is 風雨同行 (the 破傘), not 破財消災. Still TOTAL (resolves).
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;  // karma ~50
        p.SetFlag(nccu::kFlagConsoledTA);             // chose 體諒
        p.SetFlag(nccu::kFlagTaFinaleChoiceMade);
        // karma not > 80 -> not A; not cursed/cold -> not B; 體諒 -> D.
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_D);   // was: stuck Ch4 (pre-L1), C (pre-G1)
    }
    SUBCASE("體諒 + karma>80 but no TrueUmbrella -> Ending D (G1)") {
        // Also reslotted C -> D: 體諒 high-karma without the reclaimed true
        // umbrella misses A but earns the warmer D, not the buy-out C.
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100);
        p.SetFlag(nccu::kFlagConsoledTA);
        p.SetFlag(nccu::kFlagTaFinaleChoiceMade);
        // missing Flag_HasTrueUmbrella -> not A; consoled -> not cold B -> D.
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_D);   // was: stuck Ch4 (pre-L1), C (pre-G1)
    }
    SUBCASE("precedence: cold finale never overrides an earned A") {
        // 體諒 was chosen AND fully earned -> A still wins (the finale
        // flag is set but Flag_ConsoledTA is too, so coldFinale=false).
        SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
        Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
        p.AddKarma(100);
        p.SetFlag(nccu::kFlagHasTrueUmbrella);
        p.SetFlag(nccu::kFlagConsoledTA);
        p.SetFlag(nccu::kFlagTaFinaleChoiceMade);
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
        p.AddKarma(100); p.SetFlag(nccu::kFlagHasTrueUmbrella);
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

// G2 — the ending must NOT be abrupt. CheckEndingGates now DEFERS while a
// dialog / narration is on screen (the owner's 「不要突然按下選項後跳結局」),
// then resolves on a later poll once the box has CLOSED. Replaces the old
// "closes a still-active dialog on transition" contract (which snapped the
// ending the SAME frame, discarding any closing beat).
TEST_CASE("G2: ending gate DEFERS behind an active dialog, then resolves on close") {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::gfx::Vec2{0, 0}};
    nccu::DialogState d;
    d.Open({"queued closing narration line"});
    REQUIRE(d.Active());
    p.SetFlag(nccu::kFlagBoughtUglyUmbrella);

    // While the narration is up the gate must do NOTHING — the player reads
    // it first. (Pre-G2 this snapped Ending C the same frame.)
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Chapter4_Finals);   // deferred
    CHECK(d.Active());                                       // box untouched

    // Player finishes reading and closes the box; the NEXT poll resolves
    // the (persistent-flag) ending and Close()s any residual dialog.
    d.Close();
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_C);
    CHECK_FALSE(d.Active());                  // terminal screen: no stale dialog
}

// ====================================================================
// G1 — the four-ending tree A -> B -> D -> C, and the proof it is TOTAL.
// ====================================================================

// Helper: resolve a fresh Ch4 player (no active dialog) and return the
// state the gate lands on. Each row is one finale outcome.
namespace {
SemesterState ResolveCh4(int karma, bool trueUmb, bool consoled,
                         bool finaleMade, bool cursed, bool boughtUgly) {
    SemesterStateMachine m; m.Transition(SemesterState::Chapter4_Finals);
    Player p{nccu::gfx::Vec2{0, 0}}; nccu::DialogState d;
    p.AddKarma(karma - 50);                   // start is 50; reach `karma`
    if (trueUmb)    p.SetFlag(nccu::kFlagHasTrueUmbrella);
    if (consoled)   p.SetFlag(nccu::kFlagConsoledTA);
    if (finaleMade) p.SetFlag(nccu::kFlagTaFinaleChoiceMade);
    if (cursed)     p.SetFlag(nccu::kFlagTookCursedUmbrella);
    if (boughtUgly) p.SetFlag(nccu::kFlagBoughtUglyUmbrella);
    nccu::CheckEndingGates(p, m, d);
    return m.Current();
}
}  // namespace

TEST_CASE("G1: the A->B->D->C ending tree") {
    using S = SemesterState;
    // A — karma>80 + 持真傘 + 體諒.
    CHECK(ResolveCh4(100, true,  true,  true,  false, false) == S::Ending_A);
    // 體諒 + karma<=80 (-> D, not C).
    CHECK(ResolveCh4(50,  true,  true,  true,  false, false) == S::Ending_D);
    CHECK(ResolveCh4(80,  true,  true,  true,  false, false) == S::Ending_D); // boundary: 80 is NOT >80
    // 體諒 + karma>80 but no true umbrella (-> D, not A, not C).
    CHECK(ResolveCh4(100, false, true,  true,  false, false) == S::Ending_D);
    // 質問 (finale made, NOT consoled) -> cold finale -> B.
    CHECK(ResolveCh4(95,  true,  false, true,  false, false) == S::Ending_B);
    // buy-ugly only (no finale) -> C.
    CHECK(ResolveCh4(50,  false, false, false, false, true)  == S::Ending_C);
    // cursed -> B (outranks everything below A).
    CHECK(ResolveCh4(50,  false, false, false, true,  false) == S::Ending_B);
    CHECK(ResolveCh4(50,  true,  true,  true,  true,  false) == S::Ending_B); // cursed beats a would-be D
    // neg karma -> B.
    CHECK(ResolveCh4(-100, false, false, false, false, false) == S::Ending_B);
    // D outranks C: a 體諒 player who ALSO bought the ugly umbrella gets the
    // warmer D (moral choice beats a shopping decision).
    CHECK(ResolveCh4(50,  true,  true,  true,  false, true)  == S::Ending_D);
}

TEST_CASE("G1: the gate is TOTAL once the finale choice is made (no soft-lock)") {
    using S = SemesterState;
    // Sweep EVERY combination of the finale-relevant flags WITH
    // Flag_TaFinaleChoiceMade set (the only way to be "stuck"): the gate must
    // ALWAYS leave Chapter4_Finals. This is the CLAUDE.md §5 winnability
    // guarantee for the 4-ending tree (extends the L1 TOTAL proof to D).
    for (int karma : {-100, 0, 50, 80, 81, 100}) {
        for (bool trueUmb : {false, true}) {
            for (bool consoled : {false, true}) {
                for (bool cursed : {false, true}) {
                    for (bool ugly : {false, true}) {
                        const S r = ResolveCh4(karma, trueUmb, consoled,
                                               /*finaleMade=*/true, cursed, ugly);
                        CHECK(r != S::Chapter4_Finals);   // never stuck
                        CHECK(IsEndingState(r));          // always an ending
                    }
                }
            }
        }
    }
}
