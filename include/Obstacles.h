#ifndef OBSTACLES_H_
#define OBSTACLES_H_
#include "gfx/Rect.h"
#include <array>
#include <string_view>

namespace nccu::obstacles {

// Static colliders that aren't tied to a building tile — terrain features
// rendered by the base worldmap that the player shouldn't be able to
// cross. Appended to staticColliders in main.cpp alongside the building
// wall rects.
//
// River: indigo band along the north inside the trapezoid (y ~ 440-580).
// A stone bridge crosses it slightly right of centre; we leave a gap at
// x ~ 950-1050 so the player can walk over.
//
// Track perimeter: the rendered running-track oval extends well past the
// 操場 building tile's bbox, so an explicit ring of strip-colliders blocks
// the lane edges. A south gap at x [1380, 1480] lets the player step onto
// the inner field, where the 操場 trigger rect still fires the chapter 3
// entry event.
inline constexpr std::array<gfx::Rect, 7> kAll = {{
    // River — left of the bridge
    gfx::Rect{   0.0f, 410.0f,  950.0f, 180.0f},
    // River — right of the bridge
    gfx::Rect{1050.0f, 410.0f,  998.0f, 180.0f},

    // Track perimeter strips (annulus with a south gap)
    // Top strip
    gfx::Rect{1180.0f, 690.0f,  520.0f,  40.0f},
    // Left straight
    gfx::Rect{1180.0f, 730.0f,   60.0f, 160.0f},
    // Right straight
    gfx::Rect{1640.0f, 730.0f,   60.0f, 160.0f},
    // Bottom strip — left of south gap
    gfx::Rect{1180.0f, 890.0f,  200.0f,  40.0f},
    // Bottom strip — right of south gap
    gfx::Rect{1480.0f, 890.0f,  220.0f,  40.0f},
}};

// Names of building entries that should NOT contribute a wall collider —
// they exist as trigger zones only (open ground inside the rect). 操場
// is fenced off by the strips above instead; 羅馬廣場 is an open plaza.
inline constexpr std::array<std::string_view, 2> kBuildingCollisionSkip = {
    "操場", "羅馬廣場"
};

} // namespace nccu::obstacles

#endif // OBSTACLES_H_
