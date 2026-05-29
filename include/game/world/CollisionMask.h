#ifndef COLLISION_MASK_H_
#define COLLISION_MASK_H_
#include "engine/math/Vec2.h"
#include <cstdint>
#include <vector>

namespace nccu {

// Per-pixel walkability grid for baked terrain (building wall bases,
// the river, hand-painted trees / planters / the campus perimeter
// wall). One byte per world pixel: 1 = solid, 0 = walkable. The PNG is
// authored in any image editor — see tools/tiled_to_world.py for the
// solid = "anything that is not pure white and not fully transparent"
// convention — and loaded through gfx::LoadCollisionMask so raylib
// stays confined to include/gfx/. This type is raylib-free so the
// AABB resolver in Physics.h can consume it directly.
//
// An empty mask (Width()==0) reports every box walkable, which is the
// graceful-degradation state when no mask PNG is found.
class CollisionMask {
public:
    CollisionMask() = default;
    CollisionMask(int w, int h, std::vector<std::uint8_t> solid)
        : w_(w), h_(h), solid_(std::move(solid)) {}

    int  Width()  const noexcept { return w_; }
    int  Height() const noexcept { return h_; }
    bool Empty()  const noexcept { return w_ <= 0 || h_ <= 0; }

    bool Solid(int px, int py) const noexcept {
        if (Empty()) return false;
        if (px < 0) px = 0; else if (px >= w_) px = w_ - 1;
        if (py < 0) py = 0; else if (py >= h_) py = h_ - 1;
        return solid_[static_cast<std::size_t>(py) * w_ + px] != 0;
    }

    // True if any pixel under the AABB footprint is solid. The box is
    // scanned on a kStep-px lattice (always including the far edges) so a
    // wall thinner than the player can never slip between samples — the
    // whole point of a pixel-accurate mask is that the engine blocks on
    // exactly what was painted.
    bool BlockedBox(float bx, float by, float bw, float bh) const noexcept {
        if (Empty()) return false;
        constexpr float kStep = 4.0f;
        const float x1 = bx + bw, y1 = by + bh;
        for (float sy = by;; sy += kStep) {
            if (sy > y1) sy = y1;
            for (float sx = bx;; sx += kStep) {
                if (sx > x1) sx = x1;
                if (Solid(static_cast<int>(sx), static_cast<int>(sy)))
                    return true;
                if (sx >= x1) break;
            }
            if (sy >= y1) break;
        }
        return false;
    }

    bool BlockedBox(nccu::engine::math::Vec2 pos, nccu::engine::math::Vec2 size) const noexcept {
        return BlockedBox(pos.x, pos.y, size.x, size.y);
    }

private:
    int                       w_{0};
    int                       h_{0};
    std::vector<std::uint8_t> solid_;
};

// Loads the canonical terrain mask: the hand-authored
// resources/assets/maps/collision_mask.png, falling back to the
// tool-generated collision_mask_base.png (buildings + river, no props)
// when it is absent. Defined in src/TerrainMask.cpp — the one non-gfx
// TU allowed to pull raylib via gfx::LoadCollisionMask — so the model
// layer (World) stays raylib-free.
CollisionMask LoadTerrainMask();

} // namespace nccu

#endif // COLLISION_MASK_H_
