// P2 cursed-taint regression. Replaces the prior F2 ".cursed pickup applies
// exactly -30 karma" guard: as of P2 the pickup no longer takes karma in one
// shot — it increments Player::cursedTaint_ instead, and the per-chapter
// ApplyCursedTaintDecay (SceneRouter Ch2/3/4 entry) bleeds -5 * taint each
// transition. So the FIRST cursed pickup costs -5 per remaining numbered
// chapter (≤ -15 over Ch2/3/4 entries on a clean Ch1 pickup); a second
// pickup raises the rate to -10/transition; a third to -15/transition. The
// Flag_TookCursedUmbrella ending marker is still set at pickup time so the
// Ending B precondition is preserved.
//
// Revert-verify (these tests must FAIL on a pre-P2 build):
//  - CursedUmbrella::BeClaimed back to `.decreaseKarma(karmaPenalty_)`
//    → the "no karma change at pickup" check breaks (karma drops to 20).
//  - cursedTaint_ removed / ApplyCursedTaintDecay no-op → the per-chapter
//    decay checks break (karma stays at the pre-decay value).

#include "doctest/doctest.h"
#include "game/entities/CursedUmbrella.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/math/Vec2.h"
#include "game/quest/Flags.h"

using nccu::engine::math::Vec2;

TEST_CASE("P2: cursed pickup increments taint, leaves karma untouched at pickup") {
    EventBus::Instance().Clear();

    Player p{Vec2{0.0f, 0.0f}};
    const int k0 = p.GetKarma();
    CHECK(k0 == 50);                                  // GDD start
    CHECK(p.GetCursedTaint() == 0);
    CHECK_FALSE(p.HasFlag(nccu::kFlagTookCursedUmbrella));

    CursedUmbrella cursed{Vec2{0.0f, 0.0f}};
    cursed.BeClaimed(&p);
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));  // Ending B key still set
    CHECK(p.HasUmbrella());                            // bag shows cursed row
    CHECK(p.GetKarma() == k0);                        // pickup itself is karma-neutral
    CHECK(p.GetCursedTaint() == 1);                   // counter advanced

    cursed.BeClaimed(&p);                             // idempotency guard (isActive_)
    CHECK(p.GetCursedTaint() == 1);                   // NOT double-bumped
    CHECK(p.GetKarma() == k0);

    EventBus::Instance().Clear();
}

TEST_CASE("P2: ApplyCursedTaintDecay bleeds -5 * taint, no-op at taint=0") {
    EventBus::Instance().Clear();

    Player p{Vec2{0.0f, 0.0f}};
    const int k0 = p.GetKarma();
    p.ApplyCursedTaintDecay();
    CHECK(p.GetKarma() == k0);                        // taint=0 -> no change

    p.IncCursedTaint();                               // simulate one pickup
    CHECK(p.GetCursedTaint() == 1);
    p.ApplyCursedTaintDecay();
    CHECK(p.GetKarma() == k0 - 5);                    // taint=1 -> -5

    p.IncCursedTaint();                               // second pickup
    CHECK(p.GetCursedTaint() == 2);
    p.ApplyCursedTaintDecay();
    CHECK(p.GetKarma() == k0 - 5 - 10);               // taint=2 -> -10

    p.IncCursedTaint();                               // third pickup
    p.ApplyCursedTaintDecay();
    CHECK(p.GetKarma() == k0 - 5 - 10 - 15);          // taint=3 -> -15

    EventBus::Instance().Clear();
}

TEST_CASE("P2: taint persists through SetHasUmbrella(false) / chapter resets") {
    EventBus::Instance().Clear();

    Player p{Vec2{0.0f, 0.0f}};
    CursedUmbrella cursed{Vec2{0.0f, 0.0f}};
    cursed.BeClaimed(&p);
    CHECK(p.GetCursedTaint() == 1);

    // SceneRouter Ch2/3/4 entry calls SetHasUmbrella(false) to empty the
    // bag's umbrella row. Taint must SURVIVE — it is the "moral stain is
    // permanent" half of cursed, paired with the never-clearing
    // Flag_TookCursedUmbrella.
    p.SetHasUmbrella(false);
    CHECK(p.GetCursedTaint() == 1);
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));

    EventBus::Instance().Clear();
}

TEST_CASE("P2: decay clamps at the karma floor of -100") {
    EventBus::Instance().Clear();

    Player p{Vec2{0.0f, 0.0f}};
    p.AddKarma(-150);                                 // 50 - 150 -> clamped -100
    CHECK(p.GetKarma() == -100);
    p.IncCursedTaint();
    p.ApplyCursedTaintDecay();                        // would be -105 unclamped
    CHECK(p.GetKarma() == -100);                      // clamped at floor

    EventBus::Instance().Clear();
}
