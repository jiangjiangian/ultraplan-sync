#include "doctest/doctest.h"
#include "CursedUmbrella.h"
#include "ProfessorTrapUmbrella.h"
#include "TrueUmbrella.h"
#include "FragileUmbrella.h"
#include "Player.h"
#include "EventBus.h"
#include "gfx/Vec2.h"

// S5c-0 / F.9-a/b: the Ch1 "bad umbrella" claims are the SOURCE of the
// negative ripples Ch2/Ch3/Ch4 cash in (助教 -10/-15, 學霸 cold, Ending
// B). Before this they were never set, so that whole chain was dead
// content. These pin the seed at the claim site + its specificity.

TEST_CASE("ProfessorTrapUmbrella claim seeds Flag_HasProfessorTrap once") {
    EventBus::Instance().Clear();
    Player p{nccu::gfx::Vec2{0, 0}};
    CHECK_FALSE(p.HasFlag("Flag_HasProfessorTrap"));

    ProfessorTrapUmbrella trap{nccu::gfx::Vec2{0, 0}};
    trap.beClaimed(&p);
    CHECK(p.HasFlag("Flag_HasProfessorTrap"));
    CHECK(p.HasUmbrella());

    // Idempotent guard: a second claim is a no-op, the flag stays set.
    trap.beClaimed(&p);
    CHECK(p.HasFlag("Flag_HasProfessorTrap"));
}

TEST_CASE("CursedUmbrella claim seeds Flag_TookCursedUmbrella + keeps the penalty") {
    EventBus::Instance().Clear();
    Player p{nccu::gfx::Vec2{0, 0}};
    const int k0 = p.GetKarma();
    CHECK_FALSE(p.HasFlag("Flag_TookCursedUmbrella"));

    CursedUmbrella cursed{nccu::gfx::Vec2{0, 0}};
    cursed.beClaimed(&p);
    CHECK(p.HasFlag("Flag_TookCursedUmbrella"));
    CHECK(p.GetKarma() == k0 - 50);          // existing -50 penalty intact

    cursed.beClaimed(&p);                    // idempotent
    CHECK(p.HasFlag("Flag_TookCursedUmbrella"));
    CHECK(p.GetKarma() == k0 - 50);          // not double-penalised
}

TEST_CASE("The good/fragile umbrellas do NOT seed the ripple flags") {
    EventBus::Instance().Clear();
    Player p{nccu::gfx::Vec2{0, 0}};

    TrueUmbrella good{nccu::gfx::Vec2{0, 0}};
    good.beClaimed(&p);
    CHECK_FALSE(p.HasFlag("Flag_HasProfessorTrap"));
    CHECK_FALSE(p.HasFlag("Flag_TookCursedUmbrella"));

    Player q{nccu::gfx::Vec2{0, 0}};
    FragileUmbrella fragile{nccu::gfx::Vec2{0, 0}};
    fragile.beClaimed(&q);
    CHECK_FALSE(q.HasFlag("Flag_HasProfessorTrap"));
    CHECK_FALSE(q.HasFlag("Flag_TookCursedUmbrella"));
}
