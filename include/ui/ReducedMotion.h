#ifndef REDUCED_MOTION_H_
#define REDUCED_MOTION_H_
#include <algorithm>

namespace nccu {

// Pure animation gates used by the View / MessageView so the
// "reduced motion" accessibility preference (World::ReducedMotion(),
// surfaced through UMBRELLA_REDUCED_MOTION=1, audit D8 / SC 2.3.3)
// can suppress every continuous-time animation in one place. Each
// helper is a single-line free function with no raylib dependency so
// it is unit-testable headless.
//
// Default behaviour (reduced == false) is byte-equivalent to the prior
// inline math, so wiring these in does not perturb the deterministic
// autoplay harness for the harness's default flag-off case.

// Interlude exit ground marker: pixels of phase to add this frame.
// When reduced motion is on, the sweep is frozen (caller paints the
// dashes at phase 0).
constexpr float InterludeMarkerPhaseStep(float dt, bool reduced) noexcept {
    return reduced ? 0.0f : dt * 30.0f;
}

// Ending card fade-in. Default: ramp 0→1 over ~one second of delta.
// When reduced motion is on, the card is fully opaque on the first
// frame it appears (no fade), so flashing-light-sensitive players are
// not subjected to the half-second luminance ramp.
constexpr float EndingFadeAlphaStep(float currentAlpha,
                                    float dt, bool reduced) noexcept {
    return reduced ? 1.0f
                   : std::min(1.0f, currentAlpha + dt);
}

// HUD toast (DrawHudMessage) tail-end alpha factor in [0,1]. The
// default fades linearly from 1→0 over the final `fade` seconds; a
// reduced-motion player gets a hard cut at the TTL boundary (the
// toast holds fully opaque until it disappears in one frame).
constexpr float HudToastFadeT(float remaining, float fade,
                              bool reduced) noexcept {
    if (reduced) return 1.0f;
    const float t = remaining / fade;
    return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
}

} // namespace nccu

#endif // REDUCED_MOTION_H_
