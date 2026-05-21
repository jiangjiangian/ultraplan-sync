#ifndef PHYSICS_H_
#define PHYSICS_H_
#include "gfx/Vec2.h"
#include "gfx/Rect.h"
#include "world/CollisionMask.h"
#include <vector>

namespace nccu::physics {

// Axis-separated AABB movement resolver.
//
// Given a player's previous top-left position, the position they would
// occupy after Update(), the player's AABB size, a list of dynamic
// colliders and an optional static terrain mask, return the position
// the player should actually end up at.
//
// Strategy: try the X delta in isolation; if the resulting AABB overlaps
// any collider, drop it (player stays on prev.x). Then try the Y delta
// from the X-resolved position. This produces the classic JRPG "slide
// along the wall" feel: walking diagonally into a corner moves on the
// open axis only.
//
// `colliders` holds the per-frame DYNAMIC obstacles only (other actors'
// hit-boxes). Static terrain — building wall bases, the river, painted
// trees / planters / the perimeter wall — comes from `mask`, a
// pixel-accurate walkability grid; pass nullptr to skip terrain (NPC
// paths with no mask, unit tests). It is passed by const reference so
// the caller can rebuild the dynamic list each frame.
inline gfx::Vec2 ResolveMove(gfx::Vec2 prev,
                             gfx::Vec2 desired,
                             gfx::Vec2 playerSize,
                             const std::vector<gfx::Rect>& colliders,
                             const CollisionMask* mask = nullptr) {
    auto overlapsAny = [&](float x, float y) -> bool {
        const gfx::Rect aabb{x, y, playerSize.x, playerSize.y};
        for (const auto& c : colliders) {
            if (aabb.Intersects(c)) return true;
        }
        return mask && mask->BlockedBox(x, y, playerSize.x, playerSize.y);
    };

    // Escape mode: if prev is already overlapping something (spawned on
    // top of a collider, an NPC moved into us, etc.) the axis-tests below
    // would always fail and the player would soft-lock. Give them a free
    // pass to the desired position — next frame's prev is presumed clear
    // and normal blocking resumes.
    if (overlapsAny(prev.x, prev.y)) return desired;

    gfx::Vec2 out = prev;
    if (!overlapsAny(desired.x, prev.y)) out.x = desired.x;
    if (!overlapsAny(out.x, desired.y))  out.y = desired.y;
    return out;
}

} // namespace nccu::physics

#endif // PHYSICS_H_
