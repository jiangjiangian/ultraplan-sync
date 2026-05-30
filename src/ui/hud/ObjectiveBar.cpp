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

/**
 * @file ObjectiveBar.cpp
 * @brief HUD 任務指引列：在畫面頂端置中、狀態面板下方，顯示一行當前任務目標。
 */

namespace nccu {

/**
 * @brief 繪製當前任務目標的單行指引列。
 *
 * 置於頂端置中、但壓在左上狀態欄之下，避免遮住業力／章節等數值讀數；以深色底
 * 面板襯托明亮文字使其更顯眼。無玩家或目標字串為空時直接略過，不畫面板。
 */
void DrawObjectiveBar(nccu::engine::render::IRenderer& r,
                      const World& world,
                      SemesterState st,
                      float screenW,
                      float /*screenH*/) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    const Player* p = world.GetPlayer();
    if (!p) return;

    const std::string obj = CurrentObjective(st, *p);
    if (obj.empty()) return;

    constexpr int   kObjSize = 14;
    constexpr float kPad     = 6.0f;
    // 量測實際渲染後的文字尺寸，讓底色面板剛好包住它——面板會隨目標字串長短伸縮，
    // 而非由字數猜一個固定寬度。
    const Vec2 m = TextBuilder{obj}.Size(kObjSize).Measure();
    const float panelW = m.x + kPad * 2.0f;
    const float panelH = m.y + kPad * 2.0f;
    float px = screenW * 0.5f - panelW * 0.5f;
    if (px < 4.0f) px = 4.0f;
    // 坐落在左上狀態面板之下（最多 6 行約到 y132），使較寬的任務列也不會碰到
    // 業力／金錢／降雨讀數。
    constexpr float kPanelY = 138.0f;
    r.DrawRect(Rect{px, kPanelY, panelW, panelH},
               Color{20, 22, 30, 185});
    TextBuilder{obj}.At(Vec2{px + kPad, kPanelY + kPad})
        .Size(kObjSize).Color(Colors::White).Draw();
}

}  // namespace nccu
