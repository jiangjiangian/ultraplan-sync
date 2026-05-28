// Regression guard for BUGLEDGER F2 — CursedUmbrella's karma penalty
#include "quest/Flags.h"
// exceeded the locked big-event scale. The GDD / SCRIPT_HANDOFF lock
// large karma swings at −15 / −30; CursedUmbrella applied −50, 67% over
// the ceiling. Cycle 3 retunes karmaPenalty_ 50 → 30 so the heaviest
// single moral hit sits exactly on the −30 lock.
//
// Ending B reachability is UNCHANGED by this: EndingGate.cpp routes
// Ending_B on `Flag_TookCursedUmbrella || GetKarma() < 0`, and
// CursedUmbrella::beClaimed still unconditionally sets that flag — so
// the cursed path still reaches Ending B (it never relied on karma<0).
//
// Revert-verify (must FAIL without the fix): restore
// `karmaPenalty_(50)` in include/CursedUmbrella.h — the exact-30
// assertions below break (karma would drop by 50, landing at 0/-30
// instead of 20/0), as would the idempotency re-claim check.

#include "doctest/doctest.h"
#include "entities/CursedUmbrella.h"
#include "entities/Player.h"
#include "controller/EventBus.h"
#include "engine/math/Vec2.h"

using nccu::gfx::Vec2;

// F2 — a CursedUmbrella claim decreases karma by EXACTLY 30, not 50, and
// is idempotent (a second beClaimed must not re-apply the penalty: the
// isActive_ guard). Default starting karma is 50, so post-claim karma is
// 50 − 30 = 20 (it would be 0 under the old −50).
TEST_CASE("F2: CursedUmbrella applies exactly -30 karma (locked scale), idempotent") {
    EventBus::Instance().Clear();

    Player p{Vec2{0.0f, 0.0f}};
    const int k0 = p.GetKarma();
    CHECK(k0 == 50);                                  // GDD start
    CHECK_FALSE(p.HasFlag(nccu::kFlagTookCursedUmbrella));

    CursedUmbrella cursed{Vec2{0.0f, 0.0f}};
    CHECK(cursed.GetKarmaPenalty() == 30);            // locked scale, not 50

    cursed.beClaimed(&p);
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));      // Ending B key set
    CHECK(p.HasUmbrella());
    CHECK(p.GetKarma() == k0 - 30);                   // exactly -30 (==20)
    CHECK(p.GetKarma() == 20);

    cursed.beClaimed(&p);                             // idempotent re-claim
    CHECK(p.GetKarma() == k0 - 30);                   // NOT double-penalised
    CHECK(p.GetKarma() == 20);

    EventBus::Instance().Clear();
}

// F2 companion — the penalty is applied via the same fluent
// decreaseKarma path regardless of start, and karma stays clamped to the
// [-100,100] floor. From a low karma the −30 still applies linearly
// (no clamp interaction here: 5 − 30 = −25, well inside the floor).
TEST_CASE("F2: the -30 cursed penalty composes linearly with prior karma") {
    EventBus::Instance().Clear();

    Player p{Vec2{0.0f, 0.0f}};
    p.AddKarma(-45);                                  // 50 - 45 = 5
    CHECK(p.GetKarma() == 5);

    CursedUmbrella cursed{Vec2{0.0f, 0.0f}};
    cursed.beClaimed(&p);
    CHECK(p.GetKarma() == -25);                       // 5 - 30, not 5 - 50

    EventBus::Instance().Clear();
}
