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
};

} // namespace nccu::gfx
