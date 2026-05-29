#include "ui/hud/ObjectiveBar.h"

#include "game/entities/Player.h"
#include "game/quest/QuestObjective.h"
#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

#include <string>

namespace nccu {

void DrawObjectiveBar(nccu::gfx::IRenderer& r,
                      const World& world,
                      SemesterState st,
                      float screenW,
                      float /*screenH*/) {
    using namespace nccu::gfx;
    using namespace nccu::engine::math;

    // Quest objective: a panel-backed one-liner, top-centre but BELOW
    // the left status column (its lines end ~y86) so it never overlaps
    // the karma/chapter readout — the playtest's "不要佔到左上數值狀態
    // ，放一個面板當底". Smaller than the status text; the dark panel
    // makes the bright text pop ("比較顯眼"). Width is estimated from the
    // UTF-8 codepoint count (lead bytes — continuation bytes are
    // 10xxxxxx) treating each glyph as ~size wide, enough to centre a
    // one-liner without a text-measurement dependency in the View.
    const Player* p = world.GetPlayer();
    if (!p) return;

    const std::string obj = CurrentObjective(st, *p);
    if (obj.empty()) return;

    constexpr int   kObjSize = 14;
    constexpr float kPad     = 6.0f;
    // Measure the actual rendered text so the backing panel hugs it
    // exactly — it grows and shrinks with the objective string
    // instead of guessing a fixed width from a glyph count.
    const Vec2 m = TextBuilder{obj}.Size(kObjSize).Measure();
    const float panelW = m.x + kPad * 2.0f;
    const float panelH = m.y + kPad * 2.0f;
    float px = screenW * 0.5f - panelW * 0.5f;
    if (px < 4.0f) px = 4.0f;
    // Sit BELOW the top-left status panel (≤6 rows reaches ~y132),
    // so a wide objective bar never touches the karma/money/rain
    // readout — the playtest's "任務指引往下一點不要碰到數值面板".
    constexpr float kPanelY = 138.0f;
    r.DrawRect(Rect{px, kPanelY, panelW, panelH},
               Color{20, 22, 30, 185});
    TextBuilder{obj}.At(Vec2{px + kPad, kPanelY + kPad})
        .Size(kObjSize).Color(Colors::White).Draw();
}

}  // namespace nccu
