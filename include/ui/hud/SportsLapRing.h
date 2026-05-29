#ifndef UI_HUD_SPORTS_LAP_RING_H_
#define UI_HUD_SPORTS_LAP_RING_H_

namespace nccu {

class World;
namespace engine::render { class IRenderer; }

// 操場 校慶 lap progress ring (P1 step 7e — extracted from
// View::RenderHud). A 16-dot ring in the top-right that fills clockwise
// as the lap progresses; the screen companion to the ground track that
// LapTrack draws in world space.
//
// Reactive: a pure function of World::SportsLapActive +
// SportsLapProgress. Safe to call every frame — early-returns when the
// lap is inactive.
//
// Render-only (MVC §5): reads World const, never mutates. The ring is
// drawn as 16 small filled squares (no DrawCircle dependency on the
// renderer interface), keeping it spy-testable headlessly.
void DrawSportsLapRing(nccu::engine::render::IRenderer& r,
                       const World& world,
                       float screenW,
                       float screenH);

}  // namespace nccu

#endif  // UI_HUD_SPORTS_LAP_RING_H_
