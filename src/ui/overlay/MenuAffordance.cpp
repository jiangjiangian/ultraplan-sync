#include "ui/overlay/MenuAffordance.h"

#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/render/TextBuilder.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

#include <string>

/**
 * @file MenuAffordance.cpp
 * @brief HUD 右上角的選單提示：恆亮的「M 選單」小標籤，告知玩家有戲內選單可開。
 */

namespace nccu {

/**
 * @brief 繪製右上角的選單提示標籤。
 *
 * 提示遊戲存在可開啟的選單（「M 選單」）；以深色面板襯底，使其在任何地圖底磚上都
 * 清楚可讀。選單本身開啟時隱藏此提示（選單會取代它），避免重複顯示。
 */
void DrawMenuAffordance(nccu::engine::render::IRenderer& r,
                        const World& world,
                        float screenW,
                        float /*screenH*/) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    if (world.MenuOpen()) return;

    constexpr float kAffSize = 14.0f;
    constexpr float kPad     = 5.0f;
    const std::string aff = "M 選單";
    int glyphs = 0;
    for (unsigned char c : aff)
        if ((c & 0xC0) != 0x80) ++glyphs;
    // 「M 」為 1 個窄字＋選單 2 個全形寬字，故寬度估計取寬鬆值以容納全形格。
    const float tw = static_cast<float>(glyphs) * kAffSize;
    const float panelW = tw + kPad * 2.0f;
    const float panelH = kAffSize + kPad * 2.0f;
    const float px = screenW - panelW - 6.0f;
    r.DrawRect(Rect{px, 6.0f, panelW, panelH},
               Color{20, 22, 30, 170});
    TextBuilder{aff}.At(Vec2{px + kPad, 6.0f + kPad})
        .Size(static_cast<int>(kAffSize)).Color(Colors::White).Draw();
}

}  // namespace nccu
