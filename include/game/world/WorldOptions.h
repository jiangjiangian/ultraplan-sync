#ifndef WORLD_OPTIONS_H_
#define WORLD_OPTIONS_H_

namespace nccu {

// Plan P2 step 4 — kill the getenv leak in World. Pre-step the World
// ctor read UMBRELLA_REDUCED_MOTION + UMBRELLA_LARGE_TARGETS directly
// (blueprint phase-2 "Misc — World.cpp:38,51 getenv: the one impurity
// in World"). Routing those reads through this DTO keeps World pure:
// the ctor accepts the already-resolved bools instead of calling
// std::getenv itself, and main.cpp (the composition root) does the env
// resolution exactly once. Tests can construct WorldOptions{} (both
// false) so a doctest never depends on the host process's environment.
//
// Defaults match the prior behaviour-when-env-unset: both off.
struct WorldOptions {
    // Cycle 9.E (audit D8 / SC 2.3.3): the reduced-motion accessibility
    // preference. When set, View suppresses the chapter-card slide and
    // the lap-ring kinetic shimmer.
    bool reducedMotion = false;
    // Cycle 9.E (audit M2 / D7 / SC 2.5.8): the larger-targets
    // accessibility profile. When set, the E-probe reach grows from
    // 8 → 16 px on each side so tremor-affected players can trigger
    // NPC dialog without pixel-precise alignment.
    bool largeTargets = false;
};

// Resolve WorldOptions from the process environment. Used by main.cpp
// (the composition root) — keeps the env-read in ONE explicit spot
// instead of buried inside the World ctor. Accepts "1" only on each
// variable; anything else (unset/"0") leaves the default false. The
// function is pure relative to its arg list (std::getenv read), so
// tests that don't call it get a deterministic options struct.
[[nodiscard]] WorldOptions ReadWorldOptionsFromEnv();

} // namespace nccu

#endif // WORLD_OPTIONS_H_
