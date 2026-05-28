#include "doctest/doctest.h"
#include "quest/Flags.h"
#include "entities/CursedUmbrella.h"
#include "entities/ProfessorTrapUmbrella.h"
#include "entities/TrueUmbrella.h"
#include "entities/FragileUmbrella.h"
#include "entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/math/Vec2.h"

// S5c-0 / F.9-a/b: the Ch1 "bad umbrella" claims are the SOURCE of the
// negative ripples Ch2/Ch3/Ch4 cash in (助教 -10/-15, 學霸 cold, Ending
// B). Before this they were never set, so that whole chain was dead
// content. These pin the seed at the claim site + its specificity.

TEST_CASE("ProfessorTrapUmbrella claim seeds Flag_HasProfessorTrap once") {
    EventBus::Instance().Clear();
    Player p{nccu::gfx::Vec2{0, 0}};
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasProfessorTrap));

    ProfessorTrapUmbrella trap{nccu::gfx::Vec2{0, 0}};
    trap.beClaimed(&p);
    CHECK(p.HasFlag(nccu::kFlagHasProfessorTrap));
    CHECK(p.HasUmbrella());

    // Idempotent guard: a second claim is a no-op, the flag stays set.
    trap.beClaimed(&p);
    CHECK(p.HasFlag(nccu::kFlagHasProfessorTrap));
}

TEST_CASE("CursedUmbrella claim seeds Flag_TookCursedUmbrella + keeps the penalty") {
    EventBus::Instance().Clear();
    Player p{nccu::gfx::Vec2{0, 0}};
    const int k0 = p.GetKarma();
    CHECK_FALSE(p.HasFlag(nccu::kFlagTookCursedUmbrella));

    CursedUmbrella cursed{nccu::gfx::Vec2{0, 0}};
    cursed.beClaimed(&p);
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));
    CHECK(p.GetKarma() == k0 - 30);          // F2: locked -30 big-event penalty

    cursed.beClaimed(&p);                    // idempotent
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));
    CHECK(p.GetKarma() == k0 - 30);          // not double-penalised
}

// Regression — CLAUDE.md §5 red line: "Umbrella beClaimed / pickups
// keep their isActive_ idempotency guard." TrueUmbrella and
// FragileUmbrella::beClaimed previously had NO guard (unlike Cursed /
// ProfTrap above), relying solely on the caller's ForEachActiveExcept
// active-filter. OnPickup is a SECOND entry point and the contract
// requires the guard on the method itself (defense-in-depth). A
// guard-less TrueUmbrella re-publishes UmbrellaClaimed on a second
// call — which, since the Ch1/Ch3 EventWiring sibling-if advances the
// semester on that event, is a latent double-transition hazard.
TEST_CASE("TrueUmbrella::beClaimed is idempotent (no double UmbrellaClaimed)") {
    EventBus::Instance().Clear();
    int claimed = 0;
    EventBus::Instance().Subscribe(EventType::UmbrellaClaimed,
        [&claimed](const Event& e) { if (e.text == "TrueUmbrella") ++claimed; });
    Player p{nccu::gfx::Vec2{0, 0}};

    TrueUmbrella good{nccu::gfx::Vec2{0, 0}};
    good.beClaimed(&p);
    CHECK(p.HasUmbrella());
    CHECK_FALSE(good.IsActive());            // marked for the sweep
    CHECK(claimed == 1);

    good.beClaimed(&p);                       // second call: must be a no-op
    CHECK(claimed == 1);                      // NOT re-published (was 2 pre-fix)
    EventBus::Instance().Clear();
}

TEST_CASE("FragileUmbrella::beClaimed is idempotent (no double UmbrellaClaimed)") {
    EventBus::Instance().Clear();
    int claimed = 0;
    EventBus::Instance().Subscribe(EventType::UmbrellaClaimed,
        [&claimed](const Event& e) { if (e.text == "FragileUmbrella") ++claimed; });
    Player p{nccu::gfx::Vec2{0, 0}};

    FragileUmbrella fragile{nccu::gfx::Vec2{0, 0}};
    fragile.beClaimed(&p);
    CHECK(p.HasUmbrella());
    CHECK_FALSE(fragile.IsActive());
    CHECK(claimed == 1);

    fragile.beClaimed(&p);                    // second call: must be a no-op
    CHECK(claimed == 1);                      // NOT re-published (was 2 pre-fix)
    EventBus::Instance().Clear();
}

TEST_CASE("The good/fragile umbrellas do NOT seed the ripple flags") {
    EventBus::Instance().Clear();
    Player p{nccu::gfx::Vec2{0, 0}};

    TrueUmbrella good{nccu::gfx::Vec2{0, 0}};
    good.beClaimed(&p);
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasProfessorTrap));
    CHECK_FALSE(p.HasFlag(nccu::kFlagTookCursedUmbrella));

    Player q{nccu::gfx::Vec2{0, 0}};
    FragileUmbrella fragile{nccu::gfx::Vec2{0, 0}};
    fragile.beClaimed(&q);
    CHECK_FALSE(q.HasFlag(nccu::kFlagHasProfessorTrap));
    CHECK_FALSE(q.HasFlag(nccu::kFlagTookCursedUmbrella));
}
