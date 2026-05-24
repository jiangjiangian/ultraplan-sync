#include "doctest/doctest.h"
#include "quest/Chapter4Quest.h"
#include "state/EndingGate.h"
#include "state/SemesterStateMachine.h"
#include "dialog/DialogState.h"
#include "entities/Player.h"
#include "gfx/Vec2.h"

using nccu::SemesterState;

// T4 regression: the GENTLE finale (體諒, Flag_ConsoledTA) ALSO hands the
// player's true umbrella back — narratively the 助教 presses YOUR umbrella
// into your hands when you're kind. So 體諒 → Flag_ConsoledTA +
// Flag_HasTrueUmbrella → (with karma>80) Ending A, WITHOUT requiring the
// hidden Ch4 umbrella. The harsh 質問 path never sets Flag_ConsoledTA, so it
// gets no umbrella and still resolves to Ending B (coldFinale). The
// karma>80 gate for A is unchanged. TryGrantTaFinaleUmbrella is the quest-
// layer helper the GameController calls on a confirmed 助教 finale choice.

namespace {
Player MakePlayer() { return Player{nccu::gfx::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh4 = SemesterState::Chapter4_Finals;
}  // namespace

TEST_CASE("T4: 體諒 (Flag_ConsoledTA) grants Flag_HasTrueUmbrella + HasUmbrella") {
    Player p = MakePlayer();
    p.SetFlag("Flag_ConsoledTA");                 // chose 體諒
    REQUIRE_FALSE(p.HasFlag("Flag_HasTrueUmbrella"));
    REQUIRE_FALSE(p.HasUmbrella());

    nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);
    CHECK(p.HasFlag("Flag_HasTrueUmbrella"));     // Ending A's 持傘 condition
    CHECK(p.HasUmbrella());                       // physically holds it (rain)

    // Idempotent on a re-talk / re-confirm.
    nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);
    CHECK(p.HasFlag("Flag_HasTrueUmbrella"));
}

TEST_CASE("T4: the harsh / unmade finale never gets the umbrella") {
    SUBCASE("no 體諒 flag -> no grant (harsh 質問 path)") {
        Player p = MakePlayer();
        // 質問 path: Flag_TaFinaleChoiceMade is set by GameController but
        // Flag_ConsoledTA is NOT — so the gentle grant must no-op.
        p.SetFlag("Flag_TaFinaleChoiceMade");
        nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);
        CHECK_FALSE(p.HasFlag("Flag_HasTrueUmbrella"));
        CHECK_FALSE(p.HasUmbrella());
    }
    SUBCASE("wrong npc / wrong state -> no-op even with 體諒") {
        Player p = MakePlayer();
        p.SetFlag("Flag_ConsoledTA");
        nccu::TryGrantTaFinaleUmbrella(p, "victim", kCh4);          // other npc
        nccu::TryGrantTaFinaleUmbrella(
            p, "ta", SemesterState::Chapter1_AddDrop);              // not Ch4
        CHECK_FALSE(p.HasFlag("Flag_HasTrueUmbrella"));
    }
}

TEST_CASE("T4: 體諒 + karma>80 reaches Ending A WITHOUT the hidden umbrella") {
    // The whole gentle route end-to-end, in GameController's exact confirm
    // order: ApplyDialogChoice sets Flag_ConsoledTA (+15), SetFlag
    // Flag_TaFinaleChoiceMade, then TryGrantTaFinaleUmbrella grants the
    // 持傘 flag, THEN CheckEndingGates runs. The grant lands before the
    // gate, so karma>80 + 體諒 alone reach Ending A — no separate hidden
    // Ch4 umbrella pickup needed.
    nccu::SemesterStateMachine m; m.Transition(kCh4);
    Player p = MakePlayer();
    nccu::DialogState d;
    p.AddKarma(100);                              // > 80
    p.SetFlag("Flag_ConsoledTA");                 // chose 體諒 (ApplyDialogChoice)
    p.SetFlag("Flag_TaFinaleChoiceMade");         // GameController self-lock
    nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);   // gentle grant (T4)
    CHECK(p.HasFlag("Flag_HasTrueUmbrella"));
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_A);
}

TEST_CASE("T4/G1: a 體諒 player with karma<=80 still gets the umbrella (-> D, not stuck)") {
    // The grant is unconditional on the gentle branch (not karma-gated);
    // only Ending A's karma>80 gate decides A-vs-the-rest. G1: a 體諒 player
    // who never reached karma>80 now lands the bittersweet Ending D
    // (風雨同行) — reslotted from the old C — and is never stuck in Ch4.
    nccu::SemesterStateMachine m; m.Transition(kCh4);
    Player p = MakePlayer();                      // karma ~50
    nccu::DialogState d;
    p.SetFlag("Flag_ConsoledTA");
    p.SetFlag("Flag_TaFinaleChoiceMade");
    nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);
    CHECK(p.HasFlag("Flag_HasTrueUmbrella"));     // got the umbrella
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_D);   // karma<=80 -> D (was C)
}

TEST_CASE("T4: harsh 質問 (coldFinale) still routes to Ending B, not A") {
    // The harsh branch: high karma but NO 體諒. coldFinale =
    // Flag_TaFinaleChoiceMade && !Flag_ConsoledTA -> Ending B. The gentle
    // grant no-ops (no ConsoledTA), so no umbrella, so A is impossible.
    nccu::SemesterStateMachine m; m.Transition(kCh4);
    Player p = MakePlayer();
    nccu::DialogState d;
    p.AddKarma(100);
    p.SetFlag("Flag_TaFinaleChoiceMade");         // 質問 confirmed
    // GameController would call the grant; it no-ops without ConsoledTA.
    nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);
    CHECK_FALSE(p.HasFlag("Flag_HasTrueUmbrella"));
    nccu::CheckEndingGates(p, m, d);
    CHECK(m.Current() == SemesterState::Ending_B);   // coldFinale wins
}

TEST_CASE("T4: Ending B/C unaffected by the gentle-umbrella change") {
    SUBCASE("cursed -> B regardless of the new helper") {
        nccu::SemesterStateMachine m; m.Transition(kCh4);
        Player p = MakePlayer(); nccu::DialogState d;
        p.SetFlag("Flag_TookCursedUmbrella");
        nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);   // no ConsoledTA -> no-op
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_B);
    }
    SUBCASE("bought ugly -> C regardless of the new helper") {
        nccu::SemesterStateMachine m; m.Transition(kCh4);
        Player p = MakePlayer(); nccu::DialogState d;
        p.SetFlag("Flag_BoughtUglyUmbrella");
        nccu::TryGrantTaFinaleUmbrella(p, "ta", kCh4);   // no-op
        nccu::CheckEndingGates(p, m, d);
        CHECK(m.Current() == SemesterState::Ending_C);
    }
}
