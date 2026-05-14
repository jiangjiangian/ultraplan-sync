#ifndef GFX_BOUNDS_H_
#define GFX_BOUNDS_H_
#include "gfx/Vec2.h"

namespace nccu::gfx {

// Clamp `pos` so an AABB of `size` anchored at `pos` stays inside the
// [0, worldSize] box. If `size` exceeds `worldSize` on an axis, pos pins
// to 0 on that axis (the AABB is wider than the world there).
inline Vec2 ClampToWorld(Vec2 pos, Vec2 size, Vec2 worldSize) noexcept {
    auto clamp1 = [](float v, float lo, float hi) noexcept {
        if (hi < lo) return lo;   // size > world on this axis
        if (v < lo)  return lo;
        if (v > hi)  return hi;
        return v;
    };
    return Vec2{
        clamp1(pos.x, 0.0f, worldSize.x - size.x),
        clamp1(pos.y, 0.0f, worldSize.y - size.y)
    };
}

} // namespace nccu::gfx

#endif // GFX_BOUNDS_H_
