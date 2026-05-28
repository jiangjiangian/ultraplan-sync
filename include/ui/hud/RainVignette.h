#ifndef UI_HUD_RAIN_VIGNETTE_H_
#define UI_HUD_RAIN_VIGNETTE_H_

namespace nccu {

class World;
namespace gfx { class IRenderer; }

// Rain "pressure" vignette (P1 step 7e — extracted from View::RenderHud).
// Screen-edge darkening in two tiers driven by the active player's
// RainMeter — purely visual feedback (the rain is non-lethal this
// cycle). Drawn as four border bands rather than a full-screen
// texture: cheap, no per-frame alloc, deterministic and
// spy-testable headlessly.
//
// Reactive: a pure function of `World::GetPlayer()->GetRainMeter()`
// (≥60 subtle, ≥85 stronger). Early-returns when there is no Player or
// the meter is below 60. Safe to call every frame.
//
// Render-only (MVC §5): reads World const, never mutates.
void DrawRainVignette(nccu::gfx::IRenderer& r,
                      const World& world,
                      float screenW,
                      float screenH);

}  // namespace nccu

#endif  // UI_HUD_RAIN_VIGNETTE_H_
