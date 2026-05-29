#include "ui/world/SportsLapTrack.h"

#include "game/quest/Chapter3Quest.h"
#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"

#include <cmath>

namespace nccu {

void DrawSportsLapTrack(nccu::gfx::IRenderer& r, const World& world) {
    using namespace nccu::gfx;
    using namespace nccu::engine::math;

    // 操場 校慶 lap track — a dotted STADIUM outline (running-track
    // shape: top + bottom straights joined by left + right semicircles)
    // on the field the player laps; dots already passed disappear so it
    // shrinks as the lap completes (走完動態消除). Drawn HERE — right
    // after the base map, BEFORE the building/object painter's pass — so
    // it is a GROUND DECAL: 綜合院館 (which overlaps the 操場's east
    // edge) and the runners paint OVER it (layering request:
    // 地圖 → 線條 → 綜院). World space (inside the CameraScope).
    if (!world.SportsLapActive()) return;

    const float prog = world.SportsLapProgress();
    constexpr int kDots = 48;
    constexpr float kPi = 3.14159265f;
    const float cx = nccu::kSportsTrackCx, cy = nccu::kSportsTrackCy;
    const float a  = nccu::kSportsTrackHalfLen, rad = nccu::kSportsTrackR;
    const float straight = 2.0f * a, arc = kPi * rad;
    const float perim = 2.0f * straight + 2.0f * arc;
    for (int i = 0; i < kDots; ++i) {
        const float frac = static_cast<float>(i) /
                           static_cast<float>(kDots);
        if (frac < prog) continue;          // already walked → erased
        const float d = frac * perim;       // distance along perimeter
        float x, y;
        if (d < straight) {                 // top straight, L→R
            x = cx - a + d;            y = cy - rad;
        } else if (d < straight + arc) {    // right end-cap (east)
            const float th = (d - straight) / rad;        // 0..pi
            x = cx + a + rad * std::sin(th);
            y = cy - rad * std::cos(th);
        } else if (d < 2.0f * straight + arc) {  // bottom straight, R→L
            x = cx + a - (d - straight - arc);  y = cy + rad;
        } else {                            // left end-cap (west)
            const float th = (d - 2.0f * straight - arc) / rad;
            x = cx - a - rad * std::sin(th);
            y = cy + rad * std::cos(th);
        }
        r.DrawRect(Rect{x - 5.0f, y - 5.0f, 10.0f, 10.0f},
                   Color{255, 255, 255, 240});  // lane-marker white
    }
}

}  // namespace nccu
