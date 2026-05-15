#ifndef OBSTACLES_H_
#define OBSTACLES_H_
#include "gfx/Rect.h"
#include <array>
#include <string_view>

namespace nccu::obstacles {

// Static colliders that aren't tied to a building tile — terrain features
// rendered by the base worldmap that the player shouldn't be able to
// cross. Appended to the static collider set alongside building walls.
//
// River: indigo band along the north inside the trapezoid (y ~ 440-580).
// A stone bridge crosses it slightly right of centre; we leave a gap at
// x ~ 950-1050 so the player can walk over.
//
// The running track is fully walkable — the player can run the lanes and
// cut across the infield freely, so no perimeter colliders are emitted
// for it (the 操場 trigger rect still fires the chapter-3 entry event).
inline constexpr std::array<gfx::Rect, 2> kAll = {{
    // River — left of the bridge
    gfx::Rect{   0.0f, 410.0f,  950.0f, 180.0f},
    // River — right of the bridge
    gfx::Rect{1050.0f, 410.0f,  998.0f, 180.0f},
}};

// Names of building entries that should NOT contribute a wall collider —
// they exist as trigger zones only (open ground inside the rect). 操場
// is fenced off by the strips above instead; 羅馬廣場 is an open plaza.
inline constexpr std::array<std::string_view, 2> kBuildingCollisionSkip = {
    "操場", "羅馬廣場"
};

} // namespace nccu::obstacles

#endif // OBSTACLES_H_
