#ifndef UI_HUD_OBJECTIVE_BAR_H_
#define UI_HUD_OBJECTIVE_BAR_H_

#include "game/state/SemesterState.h"

namespace nccu {

class World;
namespace engine::render { class IRenderer; }

// Quest objective bar (P1 step 7e — extracted from View::RenderHud).
// A panel-backed one-liner, top-centre but BELOW the left status panel
// (≤6 rows reaches ~y132), so the chapter objective never overlaps
// karma / 金幣 / rain readouts — playtest note "任務指引往下一點不要
// 碰到數值面板".
//
// Reactive: a pure function of `CurrentObjective(st,
// *world.GetPlayer())`. Safe to call every frame — no-op when there
// is no Player or the objective string is empty.
//
// Render-only (MVC §5): reads World const, never mutates. Width is
// measured exactly via TextBuilder::Measure() so the backing panel
// grows and shrinks with the objective text.
void DrawObjectiveBar(nccu::engine::render::IRenderer& r,
                      const World& world,
                      SemesterState st,
                      float screenW,
                      float screenH);

}  // namespace nccu

#endif  // UI_HUD_OBJECTIVE_BAR_H_
