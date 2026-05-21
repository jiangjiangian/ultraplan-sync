// Cycle 9.E (audit M2 / D7 / SC 2.5.8): pin the "larger targets"
// accessibility profile plumbed through World, mirroring the
// ReducedMotion shape exactly. Three orthogonal subcases:
//
//   (a) default: World::LargeTargets() is false (existing behaviour is
//       byte-equivalent to before — the same `kInteractReach = 8.0f`
//       that every harness regression has been pinned against).
//   (b) setter: SetLargeTargets(true) flips the flag both ways; flag
//       on widens the E-probe reach to 16 px (effective talk box
//       56x56 instead of 40x40) without changing the movement collider
//       — i.e. the player still cannot walk THROUGH an NPC, only the
//       talk reach grows. This subcase asserts the FLAG state rather
//       than re-deriving the reach; the reach constant is read by
//       GameController::Update / ScriptInput::ResolvePlan from this
//       same flag, so flag-state == behaviour by construction.
//   (c) env-var path: UMBRELLA_LARGE_TARGETS=1 wired into the World
//       ctor (engine-side trigger ahead of any pause-menu UI). Mirrors
//       the UMBRELLA_REDUCED_MOTION precedent in test_reduced_motion.
//
// Revert-verify (must FAIL without the M2 fix):
//   * Remove World::LargeTargets() / SetLargeTargets() → (a)/(b) don't
//     compile.
//   * Drop the ctor env-var read in src/World.cpp → (c) fails: the flag
//     stays false even with UMBRELLA_LARGE_TARGETS=1 in the environment.

#include "doctest/doctest.h"
#include "world/World.h"
#include <cstdlib>

using nccu::World;

TEST_CASE("M2 large-targets accessibility profile: defaults + setter") {
    SUBCASE("default flag is false") {
        // Clear env carry-over from a sibling test so the ctor reads
        // "unset" → default off. The default must remain false because
        // every harness regression and every prior playtest is pinned
        // against the original 8-px reach; flipping the default would
        // perturb byte-for-byte determinism without anyone opting in.
        unsetenv("UMBRELLA_LARGE_TARGETS");
        World w("", /*loadSprites=*/false);
        CHECK_FALSE(w.LargeTargets());
    }

    SUBCASE("setter flips the flag both ways") {
        unsetenv("UMBRELLA_LARGE_TARGETS");
        World w("", /*loadSprites=*/false);
        REQUIRE_FALSE(w.LargeTargets());

        w.SetLargeTargets(true);
        CHECK(w.LargeTargets());

        // A future pause-menu UI must be able to toggle back off too —
        // it isn't a one-way latch like e.g. Flag_HasTrueUmbrella.
        w.SetLargeTargets(false);
        CHECK_FALSE(w.LargeTargets());
    }

    SUBCASE("flag is orthogonal to ReducedMotion (no cross-coupling)") {
        // The two accessibility flags are intentionally independent
        // axes — a tremor-affected player may want larger targets but
        // also want the rain vignette to keep animating; a photo-
        // sensitive player may want reduced motion but not larger
        // targets. The setters share NO state; this subcase pins
        // the independence so a future refactor doesn't conflate them.
        unsetenv("UMBRELLA_LARGE_TARGETS");
        unsetenv("UMBRELLA_REDUCED_MOTION");
        World w("", /*loadSprites=*/false);
        REQUIRE_FALSE(w.LargeTargets());
        REQUIRE_FALSE(w.ReducedMotion());

        w.SetLargeTargets(true);
        CHECK(w.LargeTargets());
        CHECK_FALSE(w.ReducedMotion());

        w.SetReducedMotion(true);
        CHECK(w.LargeTargets());                     // unchanged
        CHECK(w.ReducedMotion());
    }
}

TEST_CASE("M2 UMBRELLA_LARGE_TARGETS=1 wires the flag on at construction") {
    SUBCASE("env=1 turns the flag on") {
        setenv("UMBRELLA_LARGE_TARGETS", "1", /*overwrite=*/1);
        World w("", /*loadSprites=*/false);
        CHECK(w.LargeTargets());
        unsetenv("UMBRELLA_LARGE_TARGETS");
    }

    SUBCASE("env=0 leaves the flag off (only '1' opts in)") {
        // Same shape as UMBRELLA_REDUCED_MOTION — accept only the
        // literal "1" so an unset/"0"/"true"/empty leaves the default
        // false. This rules out accidental opt-in from a stale env.
        setenv("UMBRELLA_LARGE_TARGETS", "0", /*overwrite=*/1);
        World w("", /*loadSprites=*/false);
        CHECK_FALSE(w.LargeTargets());
        unsetenv("UMBRELLA_LARGE_TARGETS");
    }

    SUBCASE("env unset → flag stays off") {
        unsetenv("UMBRELLA_LARGE_TARGETS");
        World w("", /*loadSprites=*/false);
        CHECK_FALSE(w.LargeTargets());
    }
}
