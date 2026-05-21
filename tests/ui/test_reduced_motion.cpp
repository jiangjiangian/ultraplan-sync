// Cycle 9.E.1 (audit D8 / SC 2.3.3): pin the reduced-motion
// accessibility preference plumbed through World + the three pure
// gate helpers in include/ReducedMotion.h. Three orthogonal subcases:
//   (a) default: World::ReducedMotion() is false (existing behaviour
//       is byte-equivalent to before, so the harness is unperturbed).
//   (b) toggle: SetReducedMotion(true) flips each of the three
//       animation gates — interlude marker sweep stops advancing,
//       the ending-card alpha jumps to 1.0 on the first paint,
//       the HUD toast fade-out collapses to a hard cut (factor 1.0
//       throughout the lifetime, then early-return at TTL).
//   (c) env-var path: UMBRELLA_REDUCED_MOTION=1 wired into the
//       World ctor (engine-side trigger ahead of the pause-menu UI).
//
// Revert-verify (must FAIL without the M3 fix):
//   * Remove World::ReducedMotion() / SetReducedMotion() → (b) won't
//     compile.
//   * Make the helpers ignore their bool arg → (b)'s helper checks
//     fail (the sweep keeps advancing / alpha stays mid-ramp / the
//     fade still ramps).
//   * Drop the ctor env-var read in src/World.cpp → (c) fails because
//     ReducedMotion() stays false even with the env var set.

#include "doctest/doctest.h"
#include "ui/ReducedMotion.h"
#include "world/World.h"
#include <cstdlib>

using nccu::EndingFadeAlphaStep;
using nccu::HudToastFadeT;
using nccu::InterludeMarkerPhaseStep;
using nccu::World;

TEST_CASE("D8 reduced-motion: defaults + helper gating") {
    SUBCASE("default flag is false") {
        // Clear any env carry-over from a previous test process so the
        // ctor reads "unset" → default off. unsetenv is POSIX.
        unsetenv("UMBRELLA_REDUCED_MOTION");
        World w("", /*loadSprites=*/false);
        CHECK_FALSE(w.ReducedMotion());
    }

    SUBCASE("setter flips the flag both ways") {
        unsetenv("UMBRELLA_REDUCED_MOTION");
        World w("", /*loadSprites=*/false);
        REQUIRE_FALSE(w.ReducedMotion());

        w.SetReducedMotion(true);
        CHECK(w.ReducedMotion());

        w.SetReducedMotion(false);
        CHECK_FALSE(w.ReducedMotion());
    }

    SUBCASE("InterludeMarkerPhaseStep freezes when reduced") {
        // dt of 1/60 s normally advances ~0.5 px (30 px/s * 1/60).
        // With reduced motion on, the step is 0.0 — the dashed sweep
        // halts in place (caller still draws the marker; only the
        // animation is suppressed).
        constexpr float dt = 1.0f / 60.0f;
        CHECK(InterludeMarkerPhaseStep(dt, false) > 0.0f);
        CHECK(InterludeMarkerPhaseStep(dt, true)  == doctest::Approx(0.0f));
    }

    SUBCASE("EndingFadeAlphaStep jumps to 1.0 instantly when reduced") {
        // Default path ramps 0 → 1 over ~one second of accumulated dt.
        // Reduced-motion path returns 1.0 on the very first call,
        // skipping the half-second luminance ramp.
        constexpr float dt = 1.0f / 60.0f;
        const float ramp   = EndingFadeAlphaStep(0.0f, dt, false);
        CHECK(ramp > 0.0f);
        CHECK(ramp < 1.0f);                // mid-ramp, not snapped
        CHECK(EndingFadeAlphaStep(0.0f, dt, true) == doctest::Approx(1.0f));
        // Even from a mid-ramp value the reduced path still snaps.
        CHECK(EndingFadeAlphaStep(0.4f, dt, true) == doctest::Approx(1.0f));
    }

    SUBCASE("HudToastFadeT holds opaque when reduced (hard cut at TTL)") {
        // Default path: with kHudFade = 1.0, half-way through the fade
        // window the factor is ~0.5 (mid-ramp).
        const float ramp = HudToastFadeT(0.5f, 1.0f, false);
        CHECK(ramp > 0.4f);
        CHECK(ramp < 0.6f);
        // Reduced: full opacity until the caller's TTL early-returns.
        CHECK(HudToastFadeT(0.5f, 1.0f, true) == doctest::Approx(1.0f));
        CHECK(HudToastFadeT(0.0f, 1.0f, true) == doctest::Approx(1.0f));
    }
}

TEST_CASE("D8 reduced-motion: UMBRELLA_REDUCED_MOTION=1 wires the flag on") {
    SUBCASE("env=1 turns the flag on at construction") {
        setenv("UMBRELLA_REDUCED_MOTION", "1", /*overwrite=*/1);
        World w("", /*loadSprites=*/false);
        CHECK(w.ReducedMotion());
        unsetenv("UMBRELLA_REDUCED_MOTION");  // restore for siblings
    }

    SUBCASE("env=0 leaves the flag off (only '1' opts in)") {
        setenv("UMBRELLA_REDUCED_MOTION", "0", /*overwrite=*/1);
        World w("", /*loadSprites=*/false);
        CHECK_FALSE(w.ReducedMotion());
        unsetenv("UMBRELLA_REDUCED_MOTION");
    }

    SUBCASE("env unset → flag stays off") {
        unsetenv("UMBRELLA_REDUCED_MOTION");
        World w("", /*loadSprites=*/false);
        CHECK_FALSE(w.ReducedMotion());
    }
}
