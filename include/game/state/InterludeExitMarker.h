#ifndef INTERLUDE_EXIT_MARKER_H_
#define INTERLUDE_EXIT_MARKER_H_
#include "game/state/InterludeExit.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"
#include <cmath>
#include <vector>

/**
 * @file InterludeExitMarker.h
 * @brief 幕間南側出口的地面視覺標記：橫跨走廊的金色虛線，含布局計算與繪製輔助。
 */

namespace nccu {

// 出口地面標記。南側出口（InterludeExit.h）原為一條看不見的 y>=1900 觸發；文字／事
// 件軌已有抵達與離場提示，此處補上視覺軌。標記是一條在出口 y 橫跨可步行幕間走廊的
// 金色水平虛線，讓玩家在鏡頭框到南側帶的當下就看到離場路線。
//
// 架構比照任務給予者指示器：純 View 層、只透過 IRenderer 繪製（本標頭不引入 raylib）、
// 隨鏡頭移動（View 在其 CameraScope 內繪製，故線條跟隨世界座標）。布局計算另抽成輔助
// 函式，使回歸測試能在沒有 GL 環境下釘住幾何。
//
// 視覺規格：金色 #FFC83D（與任務指示器面板一致）外加往東南偏移 2 px 的黑色陰影，使
// 虛線在任何地磚上都清晰。虛線長 40 px、間隙 20 px（週期 60 px）。可選的 phase 參數
// 使圖樣水平位移以製作動畫（View 以每幀秒數乘速度遞推 phase）；phase 為 0 是供測試
// 使用的確定性預設。

/// @brief 世界座標中的單一虛線矩形（純幾何，不含繪製呼叫）。
struct InterludeExitMarkerDash {
    nccu::engine::math::Rect rect; ///< 此段虛線的世界座標矩形
};

/**
 * @brief 整條標記的布局輸出：落在南側走廊內的每一段虛線。
 *
 * View 以 DrawRect 逐段繪製。將每段拆成小基本圖元，讓測試能斷言「帶內至少有一段虛
 * 線」而不必做脆弱的逐像素比對。
 */
struct InterludeExitMarkerLayout {
    std::vector<InterludeExitMarkerDash> dashes; ///< 帶內的全部虛線段
};

/// @brief 虛線粗細（像素）。
inline constexpr float kInterludeMarkerThickness = 4.0f;
/// @brief 單段虛線長度（像素）。
inline constexpr float kInterludeMarkerDashLen   = 40.0f;
/// @brief 虛線間隙長度（像素）。
inline constexpr float kInterludeMarkerGapLen    = 20.0f;
/// @brief 虛線主體金色 #FFC83D，與任務給予者指示器面板一致。
inline constexpr nccu::engine::math::Color kInterludeMarkerGold{255, 200, 61, 255};
/// @brief 陰影色，半透明以免在底層地磚上挖出實心黑洞。
inline constexpr nccu::engine::math::Color kInterludeMarkerShadow{0, 0, 0, 140};

/**
 * @brief 計算出口標記的虛線段清單。
 * @param phase 圖樣水平位移（像素）；傳 0 得到供單元測試使用、與幀無關的確定性布局。
 * @return 落在出口帶內、夾鉗於帶邊界的全部虛線段。
 *
 * 標記繪於 y == kInterludeExitMinY，即出口區的北緣，使線條成為可見的門檻而非懸浮於走
 * 廊正中。phase 會先取模到 [0, period) 區間，使長時間運行不致溢位、動畫定義良好。
 */
[[nodiscard]] inline InterludeExitMarkerLayout
LayoutInterludeExitMarker(float phase = 0.0f) {
    InterludeExitMarkerLayout out{};
    const float period = kInterludeMarkerDashLen + kInterludeMarkerGapLen;
    // 將 phase 取模到 [0, period)，使長時間運行不溢位、動畫定義良好。
    float p = std::fmod(phase, period);
    if (p < 0.0f) p += period;
    // 首段自 minX - period + p 起算，使前緣隨 phase 增加從畫面左外掃入。發射時夾鉗在
    // 帶內，讓被截斷的部分虛線仍能畫出可見內容。
    const float y = kInterludeExitMinY;
    for (float x = kInterludeExitMinX - period + p;
         x < kInterludeExitMaxX;
         x += period) {
        float dashL = x;
        float dashR = x + kInterludeMarkerDashLen;
        if (dashR <= kInterludeExitMinX) continue;       // 完全在西側帶外
        if (dashL >= kInterludeExitMaxX) break;          // 完全在東側帶外
        if (dashL < kInterludeExitMinX) dashL = kInterludeExitMinX;
        if (dashR > kInterludeExitMaxX) dashR = kInterludeExitMaxX;
        out.dashes.push_back(InterludeExitMarkerDash{
            nccu::engine::math::Rect{dashL, y, dashR - dashL, kInterludeMarkerThickness}});
    }
    return out;
}

/**
 * @brief 透過注入的 IRenderer 繪製出口標記。
 * @param r     繪製目標渲染器。
 * @param phase 圖樣水平位移（像素），轉交布局計算。
 *
 * 每段虛線先畫往東南偏移 2 px 的陰影、再畫金色主體——與任務給予者指示器相同的雙趟畫
 * 法，使線條在市集南側明亮的路面地磚上仍清晰。
 */
inline void DrawInterludeExitMarker(nccu::engine::render::IRenderer& r, float phase = 0.0f) {
    const InterludeExitMarkerLayout L = LayoutInterludeExitMarker(phase);
    for (const InterludeExitMarkerDash& d : L.dashes) {
        r.DrawRect(nccu::engine::math::Rect{d.rect.x + 2.0f, d.rect.y + 2.0f,
                             d.rect.width, d.rect.height},
                   kInterludeMarkerShadow);
        r.DrawRect(d.rect, kInterludeMarkerGold);
    }
}

} // namespace nccu

#endif // INTERLUDE_EXIT_MARKER_H_
