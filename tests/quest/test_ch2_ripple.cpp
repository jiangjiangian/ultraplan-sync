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
constexpr auto kCh2 = SemesterState::Chapter2_Midterms;
}  // namespace

TEST_CASE("ResolveOpenerSubState: Ch2 西裝學長 routes by Ch1 ripple flag") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh2, p) == 0);  // (a)
    p.SetFlag(nccu::kFlagHelpedSenior);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh2, p) == 1);  // (b)

    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagScoldedSenior);
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh2, q) == 2);  // (c)
}

TEST_CASE("ResolveOpenerSubState: Ch2 助教 — ProfessorTrap outranks HelpedTA") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("ta", kCh2, p) == 0);           // (a)

    p.SetFlag(nccu::kFlagHelpedTACh1);
    CHECK(nccu::ResolveOpenerSubState("ta", kCh2, p) == 1);           // (b)

    // Both set: (c) 取代 (a)/(b) — ProfessorTrap wins (chapter2.md L225).
    p.SetFlag(nccu::kFlagHasProfessorTrap);
    CHECK(nccu::ResolveOpenerSubState("ta", kCh2, p) == 2);           // (c)

    Player q = MakePlayer();
    q.SetFlag(nccu::kFlagHasProfessorTrap);
    CHECK(nccu::ResolveOpenerSubState("ta", kCh2, q) == 2);

    // Ch1 routing untouched (the branch is Ch2-guarded).
    Player r = MakePlayer();
    r.SetFlag(nccu::kFlagHasProfessorTrap);
    CHECK(nccu::ResolveOpenerSubState(
              "ta", SemesterState::Chapter1_AddDrop, r) == 0);
}

TEST_CASE("TryApplyCh2Ripple: lands ±3 / -10 exactly once per Ch2") {
    SUBCASE("西裝學長 HelpedSenior -> +3 once") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        p.SetFlag(nccu::kFlagHelpedSenior);
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);
        CHECK(p.GetKarma() == k0 + 3);
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);   // re-talk
        CHECK(p.GetKarma() == k0 + 3);                     // not doubled
    }
    SUBCASE("西裝學長 ScoldedSenior -> karma-neutral, key set once") {
        // T1 reframe: the Ch1 (b) call-out is now RATIONAL (+3), so the
        // Ch2 "保持距離" ripple no longer claws it back with -3 — it is
        // karma-neutral (mild embarrassment, not a penalty). The once-key
        // is still set so the arc routes exactly once.
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        p.SetFlag(nccu::kFlagScoldedSenior);
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);
        CHECK(p.GetKarma() == k0);                        // no penalty
        CHECK(p.HasFlag(nccu::kFlagCh2RippledSuitSenior)); // but consumed once
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);
        CHECK(p.GetKarma() == k0);
    }
    SUBCASE("助教 ProfessorTrap -> -10 once") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        p.SetFlag(nccu::kFlagHasProfessorTrap);
        nccu::TryApplyCh2Ripple(p, "ta", kCh2);
        CHECK(p.GetKarma() == k0 - 10);
        nccu::TryApplyCh2Ripple(p, "ta", kCh2);
        CHECK(p.GetKarma() == k0 - 10);
    }
    SUBCASE("助教 HelpedTA only -> no karma, no key (info ripple)") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        p.SetFlag(nccu::kFlagHelpedTACh1);
        nccu::TryApplyCh2Ripple(p, "ta", kCh2);
        CHECK(p.GetKarma() == k0);
        CHECK_FALSE(p.HasFlag(nccu::kFlagCh2RippledTA));
    }
    SUBCASE("no ripple flag / wrong state / wrong npc -> no-op") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);          // no flag
        p.SetFlag(nccu::kFlagHelpedSenior);
        nccu::TryApplyCh2Ripple(p, "suit_senior",
                                SemesterState::Chapter1_AddDrop);  // not Ch2
        nccu::TryApplyCh2Ripple(p, "bookworm", kCh2);              // other npc
        CHECK(p.GetKarma() == k0);
    }
}

TEST_CASE("No double: opener auto-apply contributes nothing on a routed ripple") {
    nccu::dialog::SetContentDir(TEST_CONTENT_DIR);
    Player p = MakePlayer();
    nccu::DialogState d;
    p.SetFlag(nccu::kFlagHelpedSenior);          // routes suit_senior -> (b)
    const int k0 = p.GetKarma();

    // The opener routes to (b); its once-apply is guarded by
    // !HasFlag(Flag_HelpedSenior) — already held — so it adds 0 karma.
    nccu::OpenNpcDialog(d, p, "suit_senior", kCh2);
    CHECK(p.GetKarma() == k0);               // opener applied nothing
    REQUIRE(d.Active());                     // (b) line-only opened

    // Only TryApplyCh2Ripple lands the +3 — exactly once total.
    nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);
    CHECK(p.GetKarma() == k0 + 3);
}
