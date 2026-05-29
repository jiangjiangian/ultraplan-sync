// Cycle 9.B L9 — HudExpired() lets non-View consumers (the autoplay
// harness writing state.jsonl) stop echoing a toast that has aged out.
//
// Before this fix, SetHudMessage() overwrote the buffer and TickHud
// aged hudAge_ past kHudTtl but never cleared the string — so the
// View correctly stopped drawing (DrawHudMessage early-returns above
// the TTL) but the state.jsonl harness kept emitting `"hud":"…"` with
// the same stale text for the rest of the run. The diagnostic playtest
// caught this: every frame from a chapter clear onwards reported the
// umbrella line in HUD until the next ShowMessage landed (sometimes
// minutes later).
//
// HudExpired() is a pure read-only predicate (no state mutation) so
// the View's fade-out animation contract — which still observes the
// raw hudMessage_ / hudAge_ — stays byte-identical.

#include "doctest/doctest.h"
#include "ui/MessageView.h"     // kHudTtl
#include "game/world/World.h"

#include <string>

TEST_CASE("HudExpired flips to true once hudAge_ crosses kHudTtl") {
    nccu::World w{"", /*loadSprites=*/false};

    // Fresh world: no toast set yet, so HudExpired() is false (a
    // never-set HUD is not "expired" — it has no content to expire).
    CHECK_FALSE(w.HudExpired());
    CHECK(w.HudMessage().empty());

    w.SetHudMessage("章節清關");
    CHECK_FALSE(w.HudExpired());           // just published, age = 0
    CHECK(w.HudMessage() == "章節清關");

    // 40 ticks of 0.1s ≈ 4.0s; the brief specifies this cadence but
    // float accumulation may leave hudAge_ a hair below kHudTtl (40 *
    // 0.1f sums to 3.99999... on some libm builds). Add one extra
    // tick past 4 seconds so we land cleanly above kHudTtl regardless
    // of rounding — the on-screen TTL is the contract, not exact
    // 0.1f-multiplication equality. The boundary is inclusive
    // (DrawHudMessage early-returns on age >= kHudTtl so the View has
    // already gone blank). HudExpired matches that boundary so the
    // harness and View agree on "no longer visible".
    for (int i = 0; i < 41; ++i) w.TickHud(0.1f);

    CHECK(w.HudExpired());
    // The View's fade animation depends on the message + age staying
    // around for the final kHudFade seconds; we intentionally do NOT
    // clear hudMessage_ here. HudExpired() is the only signal the
    // harness reads.
    CHECK(w.HudMessage() == "章節清關");
    CHECK(w.HudAge() >= nccu::kHudTtl);
}

TEST_CASE("A fresh SetHudMessage resets age and clears expiry") {
    nccu::World w{"", /*loadSprites=*/false};

    w.SetHudMessage("first");
    // Age past TTL so HudExpired = true.
    w.TickHud(nccu::kHudTtl + 0.5f);
    REQUIRE(w.HudExpired());

    // A new ShowMessage publish (via SetHudMessage) re-anchors the
    // banner: age resets to 0 and HudExpired flips back to false. The
    // production wiring (WireHudMessageSubscriber) calls SetHudMessage
    // on every EventType::ShowMessage so a chapter / karma / vendor
    // toast cleanly takes over from a stale string.
    w.SetHudMessage("second");
    CHECK_FALSE(w.HudExpired());
    CHECK(w.HudMessage() == "second");
    CHECK(w.HudAge() == doctest::Approx(0.0f));
}

TEST_CASE("HudExpired ignores an empty message buffer") {
    // A World that has never seen a toast (or whose message was
    // explicitly cleared) is not "expired" — there is nothing to
    // expire. This guarantees the harness emits "" both when the HUD
    // legitimately holds nothing AND when an expired toast has been
    // suppressed; the two cases collapse to the same wire format.
    nccu::World w{"", /*loadSprites=*/false};
    w.TickHud(nccu::kHudTtl + 100.0f);   // way past TTL, but no message
    CHECK_FALSE(w.HudExpired());
    CHECK(w.HudMessage().empty());
}
