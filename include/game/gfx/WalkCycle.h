#ifndef GFX_WALK_CYCLE_H_
#define GFX_WALK_CYCLE_H_
#include "engine/math/Vec2.h"
#include <array>
#include <cmath>
#include <cstddef>

// Pure, header-only Pipoya walk-sheet cell maths — the single tested
// source of truth for "which 32x32 cell of a 96x128 sheet do I blit?".
// Player.cpp pioneered this exact maths inline (Player.cpp:17-29); NPC.cpp
// (U1-T3) now shares it so a wandering / 校慶-runner NPC animates the SAME
// way the Player does. Pure functions of an animation step + a heading —
// no raylib, no GL, no sim state — so the cell selection is unit-testable
// headlessly even though NPC::Render's textured blit is not (a headless
// test has no GL context, so Texture::IsValid() is false there).
namespace nccu::gfx {

// One Pipoya cell is 32x32; a sheet is 3 columns (walk frames) x 4 rows
// (facing). The walk strip cycles idle(1) -> left-foot(0) -> idle(1) ->
// right-foot(2) so a 4-step counter ping-pongs the two stride frames
// through the idle frame, reading as a natural footfall.
inline constexpr int   kPipoyaCell        = 32;
inline constexpr float kWalkFrameDuration  = 0.15f;  // seconds per step
inline constexpr std::array<int, 4> kWalkColumns = {1, 0, 1, 2};

// Column for animation step `s` (any integer; wrapped into [0,4)). Step 0
// is the idle column, so a freshly-reset / at-rest sprite shows the idle
// pose. Used by both the moving (advancing) and resting (step 0) cases.
[[nodiscard]] constexpr int WalkColumn(int s) noexcept {
    const std::size_t i = static_cast<std::size_t>(((s % 4) + 4) % 4);
    return kWalkColumns[i];
}

// Pipoya facing-row from a heading vector: 0=down, 1=left, 2=right, 3=up.
// Dominant axis wins (|x| vs |y|); ties resolve to the vertical axis so a
// perfect diagonal still faces up/down (matches Player.cpp:24-29). A zero
// vector returns row 0 (down) — the canonical "face the camera" rest pose.
[[nodiscard]] constexpr int WalkRowForFacing(Vec2 facing) noexcept {
    const float ax = facing.x < 0.0f ? -facing.x : facing.x;
    const float ay = facing.y < 0.0f ? -facing.y : facing.y;
    if (ax > ay) return facing.x < 0.0f ? 1 : 2;
    return facing.y < 0.0f ? 3 : 0;
}

} // namespace nccu::gfx

#endif // GFX_WALK_CYCLE_H_
