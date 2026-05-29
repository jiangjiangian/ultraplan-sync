#ifndef GFX_SPRITE_STRIP_H_
#define GFX_SPRITE_STRIP_H_
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "game/state/SemesterState.h"
#include <cmath>

// Pure (raylib-free, GL-free, sim-free) sprite-strip animation helpers.
//
// A "strip" is one horizontal PNG holding N equal-width frames in a single
// row (frame i occupies texels [i*frameW, (i+1)*frameW) on the x axis,
// full height). It is loaded as ONE texture and animated by slicing a
// per-frame source rectangle with DrawTexturePro — the raylib idiom (see
// the engine's textures_sprite_anim example, distilled in
// .claude/kb/raylib-core.md). This header owns ONLY the maths: which frame
// to show at a given time, and the source rect for that frame. The actual
// blit lives in the View through IRenderer::DrawSprite, so Items/Model
// code never touch raylib (architecture red line) and a missing texture
// simply draws nothing.
//
// Everything here is constexpr / pure, so the frame logic is unit-tested
// headless with no GL context — exactly like Camera2D::ClampToWorld.

namespace nccu::gfx {

// FrameAt: the PING-PONG (triangle-wave) frame index at elapsed time `t`
// (seconds) for an `n`-frame strip advancing at `fps` frames/second.
//
// The sequence is forward-then-backward and loops:
//     0,1,2,...,n-1,n-2,...,1,  0,1,2,...        (period 2*(n-1) ticks)
// e.g. n=4 yields 0,1,2,3,2,1,0,1,2,3,2,1,... — the "breathing" / 放大縮小
// look the owner asked for when the artwork zooms in across its frames.
//
// A plain forward LOOP (0,1,..,n-1,0,..) would SNAP from the last frame
// back to the first; ping-pong eases back through the frames so a
// scale-up strip reads as a smooth in-out pulse.
//
// Degenerate inputs are total and safe: n<=1 (or fps<=0, or non-finite t)
// returns frame 0 — a single-frame "strip" is just a static sprite, and a
// stopped/garbage clock holds frame 0 rather than indexing out of range.
// Negative `t` is handled too (the modulo is taken on a non-negative
// representative) so an unusual clock can never produce a negative index.
[[nodiscard]] inline int FrameAt(double t, int n, double fps) noexcept {
    if (n <= 1 || fps <= 0.0 || !std::isfinite(t)) return 0;
    // Whole-frame tick count since t=0. floor() (not truncation) keeps the
    // step monotone across t==0 for negative inputs.
    const double ticksF = std::floor(t * fps);
    // 2*(n-1) is the ping-pong period in ticks (n-1 going out, n-1 back).
    const long period = 2L * (static_cast<long>(n) - 1L);
    // Reduce into [0, period) without UB on negative values: C++ % can be
    // negative, so fold it back up by one period when it is.
    long k = static_cast<long>(ticksF);
    long m = k % period;
    if (m < 0) m += period;
    // Triangle fold: the first half [0, n-1] counts up; the second half
    // mirrors back down. At m==n-1 (the apex) we return n-1; at m==period
    // we would return 0 again, but m never reaches `period` (it is < period).
    return (m < n) ? static_cast<int>(m)
                   : static_cast<int>(period - m);
}

// The source sub-rectangle (in texels) for frame `index` of an
// `frameCount`-frame strip whose loaded texture is `texW` x `texH` pixels.
// Frames tile left-to-right in one row, so each is texW/frameCount wide and
// the full texture tall. `index` is assumed already in [0, frameCount)
// (FrameAt guarantees this); frameCount<=0 degenerates to the whole texture.
[[nodiscard]] inline nccu::engine::math::Rect StripSourceRect(int index, int frameCount,
                                          int texW, int texH) noexcept {
    if (frameCount <= 0) {
        return nccu::engine::math::Rect{0.0f, 0.0f, static_cast<float>(texW),
                    static_cast<float>(texH)};
    }
    const float frameW = static_cast<float>(texW) /
                         static_cast<float>(frameCount);
    return nccu::engine::math::Rect{static_cast<float>(index) * frameW, 0.0f,
                frameW, static_cast<float>(texH)};
}

// One placed animated decoration — PURE DATA (no texture handle, no
// raylib). The View pairs each def with the texture it loaded (by index)
// and animates it from the render clock; World never sees these, so the
// harness timeline / state.jsonl is byte-unchanged. Frame COUNT travels
// with the def (the simplest robust convention — no filename parsing, no
// sidecar file that can go missing); the frame SIZE is derived at draw
// time from the loaded texture's pixel size (texW/frameCount x texH), so
// the strip can be re-exported at any resolution without touching code.
struct DecorationDef {
    SemesterState chapter;     // drawn ONLY while the FSM is in this state
    nccu::engine::math::Vec2          center;      // world-space CENTRE the sprite is drawn around
    const char*   stripPath;   // PNG path; missing file => draws nothing
    int           frameCount;  // N frames in the horizontal strip (>=1)
    float         drawScale;   // on-screen size of ONE frame's longer side, px
    double        fps;         // ping-pong advance rate
};

// The on-screen destination rect for a decoration centred at `center`,
// given the loaded texture's pixel size and the def's draw scale. The
// frame's aspect ratio (frameW:texH) is preserved and its LONGER side is
// scaled to `drawScale` px; the rect is centred on `center` so the
// ping-pong zoom pulses symmetrically about the anchor (a statue/cat does
// not drift as it breathes). Pure — no raylib, unit-testable.
[[nodiscard]] inline nccu::engine::math::Rect DecorationDestRect(const DecorationDef& d,
                                             int texW, int texH) noexcept {
    const int n = d.frameCount > 0 ? d.frameCount : 1;
    const float frameW = static_cast<float>(texW) / static_cast<float>(n);
    const float frameH = static_cast<float>(texH);
    const float longer = frameW > frameH ? frameW : frameH;
    const float scale  = longer > 0.0f ? d.drawScale / longer : 0.0f;
    const float w = frameW * scale;
    const float h = frameH * scale;
    return nccu::engine::math::Rect{d.center.x - w * 0.5f, d.center.y - h * 0.5f, w, h};
}

} // namespace nccu::gfx

#endif // GFX_SPRITE_STRIP_H_
