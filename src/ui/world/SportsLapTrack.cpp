#include "ui/world/SportsLapTrack.h"

#include "game/quest/Chapter3Quest.h"
#include "game/world/World.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"

#include <cmath>

/**
 * @file SportsLapTrack.cpp
 * @brief 操場校慶繞圈跑道：在地面以點狀體育場外形（跑道形）描出繞圈路徑，隨繞圈完成而消減。
 */

namespace nccu {

/**
 * @brief 繪製操場校慶繞圈的地面跑道。
 *
 * 以點狀的體育場外形（上下兩段直線，左右兩端各接一個半圓）標出玩家要繞行的路徑；
 * 已通過的點消失，故跑道隨繞圈完成而縮短（走完動態消除）。繞圈未啟動時直接略過。
 *
 * 必須在「底圖之後、建築／物件繪製之前」呼叫，使其成為地面貼花：綜合院館（與操場東緣
 * 重疊）及跑者會疊繪於其上（圖層順序：地圖 → 線條 → 綜院）。座標為世界座標（在
 * CameraScope 範圍內）。
 */
void DrawSportsLapTrack(nccu::engine::render::IRenderer& r, const World& world) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;

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
        if (frac < prog) continue;          // 已走過的點消除
        const float d = frac * perim;       // 沿周長累積的距離
        float x, y;
        if (d < straight) {                 // 上方直線，左→右
            x = cx - a + d;            y = cy - rad;
        } else if (d < straight + arc) {    // 右端半圓（東側）
            const float th = (d - straight) / rad;        // 0..pi
            x = cx + a + rad * std::sin(th);
            y = cy - rad * std::cos(th);
        } else if (d < 2.0f * straight + arc) {  // 下方直線，右→左
            x = cx + a - (d - straight - arc);  y = cy + rad;
        } else {                            // 左端半圓（西側）
            const float th = (d - 2.0f * straight - arc) / rad;
            x = cx - a - rad * std::sin(th);
            y = cy + rad * std::cos(th);
        }
        r.DrawRect(Rect{x - 5.0f, y - 5.0f, 10.0f, 10.0f},
                   Color{255, 255, 255, 240});  // 跑道標線白
    }
}

}  // namespace nccu
