#pragma once
#include "gfx/Vec2.h"

namespace nccu::gfx {

// Adapter for raylib's ::Camera2D. Field names mirror raylib so the mapping
// is obvious. Fluent setters allow in-frame chaining:
//     cam.Follow(playerPos, screenCenter).WithZoom(1.0f);
struct Camera2D {
    Vec2  offset{0.0f, 0.0f};
    Vec2  target{0.0f, 0.0f};
    float rotation{0.0f};
    float zoom{1.0f};

    Camera2D& Follow(Vec2 worldTarget, Vec2 screenCenter) noexcept {
        target = worldTarget;
        offset = screenCenter;
        return *this;
    }
    Camera2D& WithZoom(float z) noexcept     { zoom = z;     return *this; }
    Camera2D& WithRotation(float r) noexcept { rotation = r; return *this; }

    // Clamp .target so a viewport of `viewportSize` centred on .target
    // stays inside [0, worldSize]. If the world is smaller than the
    // viewport on an axis, target pins to the world midpoint on that axis.
    Camera2D& ClampToWorld(Vec2 worldSize, Vec2 viewportSize) noexcept {
        auto clampAxis = [](float v, float half, float worldExtent) noexcept {
            if (worldExtent < 2.0f * half) return worldExtent * 0.5f;
            if (v < half)                  return half;
            if (v > worldExtent - half)    return worldExtent - half;
            return v;
        };
        target.x = clampAxis(target.x, viewportSize.x * 0.5f, worldSize.x);
        target.y = clampAxis(target.y, viewportSize.y * 0.5f, worldSize.y);
        return *this;
    }
};

} // namespace nccu::gfx
