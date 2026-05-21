#include "doctest/doctest.h"
#include "quest/Chapter2Quest.h"
#include "dialog/DialogOpener.h"
#include "dialog/DialogSource.h"
#include "dialog/DialogState.h"
#include "entities/Player.h"
#include "gfx/Vec2.h"

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::SemesterState;

namespace {
Player MakePlayer() { return Player{nccu::gfx::Vec2{0.0f, 0.0f}}; }
constexpr auto kCh2 = SemesterState::Chapter2_Midterms;
}  // namespace

TEST_CASE("ResolveOpenerSubState: Ch2 西裝學長 routes by Ch1 ripple flag") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh2, p) == 0);  // (a)
    p.SetFlag("Flag_HelpedSenior");
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh2, p) == 1);  // (b)

    Player q = MakePlayer();
    q.SetFlag("Flag_ScoldedSenior");
    CHECK(nccu::ResolveOpenerSubState("suit_senior", kCh2, q) == 2);  // (c)
}

TEST_CASE("ResolveOpenerSubState: Ch2 助教 — ProfessorTrap outranks HelpedTA") {
    Player p = MakePlayer();
    CHECK(nccu::ResolveOpenerSubState("ta", kCh2, p) == 0);           // (a)

    p.SetFlag("Flag_HelpedTA_Ch1");
    CHECK(nccu::ResolveOpenerSubState("ta", kCh2, p) == 1);           // (b)

    // Both set: (c) 取代 (a)/(b) — ProfessorTrap wins (chapter2.md L225).
    p.SetFlag("Flag_HasProfessorTrap");
    CHECK(nccu::ResolveOpenerSubState("ta", kCh2, p) == 2);           // (c)

    Player q = MakePlayer();
    q.SetFlag("Flag_HasProfessorTrap");
    CHECK(nccu::ResolveOpenerSubState("ta", kCh2, q) == 2);

    // Ch1 routing untouched (the branch is Ch2-guarded).
    Player r = MakePlayer();
    r.SetFlag("Flag_HasProfessorTrap");
    CHECK(nccu::ResolveOpenerSubState(
              "ta", SemesterState::Chapter1_AddDrop, r) == 0);
}

TEST_CASE("TryApplyCh2Ripple: lands ±3 / -10 exactly once per Ch2") {
    SUBCASE("西裝學長 HelpedSenior -> +3 once") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        p.SetFlag("Flag_HelpedSenior");
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);
        CHECK(p.GetKarma() == k0 + 3);
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);   // re-talk
        CHECK(p.GetKarma() == k0 + 3);                     // not doubled
    }
    SUBCASE("西裝學長 ScoldedSenior -> -3 once") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        p.SetFlag("Flag_ScoldedSenior");
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);
        CHECK(p.GetKarma() == k0 - 3);
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);
        CHECK(p.GetKarma() == k0 - 3);
    }
    SUBCASE("助教 ProfessorTrap -> -10 once") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        p.SetFlag("Flag_HasProfessorTrap");
        nccu::TryApplyCh2Ripple(p, "ta", kCh2);
        CHECK(p.GetKarma() == k0 - 10);
        nccu::TryApplyCh2Ripple(p, "ta", kCh2);
        CHECK(p.GetKarma() == k0 - 10);
    }
    SUBCASE("助教 HelpedTA only -> no karma, no key (info ripple)") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        p.SetFlag("Flag_HelpedTA_Ch1");
        nccu::TryApplyCh2Ripple(p, "ta", kCh2);
        CHECK(p.GetKarma() == k0);
        CHECK_FALSE(p.HasFlag(nccu::kFlagCh2RippledTA));
    }
    SUBCASE("no ripple flag / wrong state / wrong npc -> no-op") {
        Player p = MakePlayer();
        const int k0 = p.GetKarma();
        nccu::TryApplyCh2Ripple(p, "suit_senior", kCh2);          // no flag
        p.SetFlag("Flag_HelpedSenior");
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
    p.SetFlag("Flag_HelpedSenior");          // routes suit_senior -> (b)
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
