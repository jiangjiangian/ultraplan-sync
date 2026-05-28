#ifndef UI_WORLD_SPORTS_LAP_TRACK_H_
#define UI_WORLD_SPORTS_LAP_TRACK_H_

namespace nccu {

class World;
namespace gfx { class IRenderer; }

// 操場 校慶 lap track ground decal (P1 step 7f — extracted from
// View::RenderWorld). A dotted STADIUM outline (running-track shape:
// top + bottom straights joined by left + right semicircles) on the
// field the player laps; dots already passed disappear so the
// outline shrinks as the lap completes (走完動態消除).
//
// Drawn INSIDE a `CameraScope` by the caller so this is world space.
// Must be drawn BEFORE the painter's-order pass so 綜合院館 (which
// overlaps the 操場's east edge) and the runners paint OVER it: the
// layering request "地圖 → 線條 → 綜院". Reactive: a pure function of
// `World::SportsLapActive()` + `SportsLapProgress()`. Early-returns
// when the lap is inactive.
//
// Render-only (MVC §5). Read-only world.
void DrawSportsLapTrack(nccu::gfx::IRenderer& r, const World& world);

}  // namespace nccu

#endif  // UI_WORLD_SPORTS_LAP_TRACK_H_
