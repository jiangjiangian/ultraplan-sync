#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter2Quest.h"
#include "game/dialog/DialogOpener.h"
#include "game/dialog/DialogSource.h"
#include "game/dialog/DialogState.h"
#include "game/entities/Player.h"
#include "engine/math/Vec2.h"

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh4 = SemesterState::Chapter4_Finals;
}  // namespace

TEST_CASE("ResolveOpenerSubState: Ch4 西裝學長 splits by HelpedSenior + karma") {
    Player p = MakePlayer();
    // !HelpedSenior -> (a) (不出場 is a known omission; degrade).
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh4, p) == 0);

    p.SetFlag(nccu::kFlagHelpedSenior);
    p.AddKarma(100);                                  // clamp ~100 (>70)
    CHECK(p.GetKarma() > 70);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh4, p) == 1);  // (b)

    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagHelpedSenior);
    q.AddKarma(-100);                                 // < 30
    CHECK(q.GetKarma() < 30);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh4, q) == 2);  // (c)

    Player r = MakePlayer();                          // default karma ~50
    r.SetFlag(nccu::kFlagHelpedSenior);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh4, r) == 0);  // (a) mid
}

TEST_CASE("ResolveOpenerSubState: Ch4 助教 (b)/(c) precedence + bookworm/victim") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("ta", kCh4, p) == 0);           // (a)
    p.SetFlag(nccu::kFlagHasProfessorTrap);
    CHECK(nccu::ResolveOpenerSubState("ta", kCh4, p) == 2);           // (c)
    p.SetFlag(nccu::kFlagHelpedTACh1);
    CHECK(nccu::ResolveOpenerSubState("ta", kCh4, p) == 1);  // (b) 優先 over (c)

    Player b = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("bookworm", kCh4, b) == 2);     // 未救
    b.SetFlag(nccu::kFlagBookwormRecovered);
    CHECK(nccu::ResolveOpenerSubState("bookworm", kCh4, b) == 1);     // 救回

    Player v = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("victim", kCh4, v) == 0);       // 釋懷
    v.SetFlag(nccu::kFlagPromisedVictim);
    CHECK(nccu::ResolveOpenerSubState("victim", kCh4, v) == 1);       // 淡漠
    v.SetHasUmbrella(true);                                           // 傘已在手
    CHECK(nccu::ResolveOpenerSubState("victim", kCh4, v) == 0);       // 釋懷
}

TEST_CASE("OpenNpcDialog: Ch4 助教 (c) routing must NOT spuriously set HelpedTA") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    Player p = MakePlayer();
    nccu::DialogState d;
    p.SetFlag(nccu::kFlagHasProfessorTrap);               // routes 助教 -> (c)
    const int k0 = p.GetKarma();

    nccu::OpenNpcDialog(d, p, "ta", kCh4);
    REQUIRE(d.Active());                               // (c) opened line-only
    // chapter4.md L235's precedence prose carries
    // `Flag_HelpedTA_Ch1 = true`, which the parser records as (c)'s
    // setsFlag. The auto-apply is Ch1-scoped, so routing to Ch4 (c)
    // must NOT grant HelpedTA_Ch1 nor move karma.
    CHECK_FALSE(p.HasFlag(nccu::kFlagHelpedTACh1));
    CHECK(p.GetKarma() == k0);
}

TEST_CASE("OpenNpcDialog: Ch1 1b-3 ta reward recap still applies (guard intact)") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    Player p = MakePlayer();
    nccu::DialogState d;
    p.SetFlag(nccu::kFlagFoundForm);                       // routes Ch1 ta -> 1
    const int k0 = p.GetKarma();

    nccu::OpenNpcDialog(d, p, "ta", SemesterState::Chapter1_AddDrop);
    REQUIRE(d.Active());
    // Ch1 reward recap still fires (the Chapter1_AddDrop scoping keeps
    // the 1b-3 mechanism alive). Assert the INVARIANT — the auto-apply
    // did something (karma moved or a flag was granted) — not the exact
    // reward flag name (test_dialog_opener owns the Ch1 specifics).
    CHECK((p.GetKarma() != k0 || p.HasFlag(nccu::kFlagHelpedTACh1)));
}
