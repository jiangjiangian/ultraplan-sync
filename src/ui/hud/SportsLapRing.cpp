#include "ui/hud/SportsLapRing.h"

#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"

#include <cmath>

/**
 * @file SportsLapRing.cpp
 * @brief HUD 操場校慶繞圈進度環：螢幕座標的點狀圓環，隨繞圈完成順時針填滿。
 */

namespace nccu {

/**
 * @brief 繪製操場校慶繞圈的進度環。
 *
 * 畫在 HUD 螢幕座標，是地面繞圈跑道的螢幕對應物；繞圈未啟動時直接略過。圓環自頂端
 * 起、順時針排佈各點，frac 小於進度者填亮金色（已完成）、其餘為半透明白色（未完成）。
 */
void DrawSportsLapRing(nccu::engine::render::IRenderer& r,
                       const World& world,
                       float screenW,
                       float /*screenH*/) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

    if (!world.SportsLapActive()) return;

    const float prog = world.SportsLapProgress();
    constexpr int kDots = 16;
    constexpr float kTwoPi = 6.2831853f;
    const float cx = screenW - 60.0f, cy = 120.0f, ringR = 24.0f;
    for (int i = 0; i < kDots; ++i) {
        const float frac = static_cast<float>(i) /
                           static_cast<float>(kDots);
        const float a = -kTwoPi * 0.25f + frac * kTwoPi;  // 自頂端起、順時針
        const float x = cx + ringR * std::cos(a);
        const float y = cy + ringR * std::sin(a);
        r.DrawRect(Rect{x - 3.0f, y - 3.0f, 6.0f, 6.0f},
                   frac < prog ? Color{255, 230, 90, 255}
                               : Color{255, 255, 255, 70});
    }
}

}  // namespace nccu
