#ifndef QUEST_GIVER_INDICATOR_H_
#define QUEST_GIVER_INDICATOR_H_
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"

/**
 * @file QuestGiverIndicator.h
 * @brief 任務給予者 NPC 頭頂「!」提示的版面計算與繪製（純 View 層、可測試）。
 */

namespace nccu {

/**
 * @brief 任務給予者 NPC 頭頂「!」提示的版面資料。
 *
 * 純 View 層：吃 NPC 的碰撞盒（24x24，世界座標），畫出一個帶底板的小驚嘆號，
 * 讓玩家一眼看出可對話的劇情鉤子。不依賴 raylib、不存取 World——所有繪圖原語
 * 都經由 IRenderer，故 headless 測試的偵測替身能完整記錄。由 View 畫在
 * CameraScope 內，使提示跟著 NPC 一起位於世界座標空間。
 *
 * 版面：一個 16x16 金色方塊（#FFC83D，約等於 raylib Gold），水平置中於碰撞盒
 * 頂邊，並抬升至 NPC sprite 頂端之上 20 px（sprite 以碰撞盒底邊對齊、高 32，故
 * 其頂端位於世界座標 hitBox.y - 8）。「!」字形落在方塊正中央。
 */
struct QuestGiverIndicatorLayout {
    nccu::engine::math::Rect panel;     ///< 底板方塊
    nccu::engine::math::Vec2 textPos;   ///< 方塊內「!」的左上角座標
    int       textSize;  ///< 「!」的字體大小
};

/**
 * @brief 計算指定 NPC 碰撞盒對應的提示版面落點。
 * @param hitBox NPC 的碰撞盒（世界座標）。
 * @return 該 NPC 對應的提示版面資料。
 *
 * 自 DrawQuestGiverIndicator 抽出，讓測試能在不需模擬渲染器的情況下抽查幾何。
 */
[[nodiscard]] inline QuestGiverIndicatorLayout
LayoutQuestGiverIndicator(nccu::engine::math::Rect hitBox) noexcept {
    constexpr float kSize    = 16.0f;
    constexpr float kLiftPx  = 20.0f;  // sprite 頂端與圖示底端之間的間隙
    constexpr float kSpriteH = 32.0f;  // Pipoya 圖格高；NPC::Render 的對齊基準
    // NPC sprite 以碰撞盒底邊對齊（見 NPC::Render）：其頂端位於
    // hitBox.y + hitBox.height - kSpriteH。再往上抬一段，讓圖示浮在頭頂上方。
    const float spriteTopY = hitBox.y + hitBox.height - kSpriteH;
    const float iconY      = spriteTopY - kLiftPx - kSize;
    const float iconX      = hitBox.x + (hitBox.width - kSize) * 0.5f;
    QuestGiverIndicatorLayout out{};
    out.panel    = nccu::engine::math::Rect{iconX, iconY, kSize, kSize};
    out.textSize = 14;
    // raylib 點陣字型：「!」在字體大小 14 時約寬 3 px、高 10 px。
    // 往方塊內推一點，使字形看起來置中。
    out.textPos  = nccu::engine::math::Vec2{iconX + 6.0f, iconY + 1.0f};
    return out;
}

/**
 * @brief 透過注入的 IRenderer 繪製提示圖示。
 * @param r      繪圖介面。
 * @param hitBox NPC 的碰撞盒（世界座標）。
 *
 * 底板下方畫一道淺陰影，使其在明亮地磚上仍突出可辨。
 */
inline void DrawQuestGiverIndicator(nccu::engine::render::IRenderer& r,
                                    nccu::engine::math::Rect hitBox) {
    const QuestGiverIndicatorLayout L = LayoutQuestGiverIndicator(hitBox);
    // 投影陰影：同一矩形往右下偏移 2 px，半透明深色。
    r.DrawRect(nccu::engine::math::Rect{L.panel.x + 2.0f, L.panel.y + 2.0f,
                         L.panel.width, L.panel.height},
               nccu::engine::math::Color{0, 0, 0, 140});
    // 金色方塊——在任何地磚上都清晰可讀。
    r.DrawRect(L.panel, nccu::engine::math::Color{255, 200, 61, 255});
    // 置中於方塊內的黑色「!」字形。
    r.DrawText("!", L.textPos, L.textSize, nccu::engine::math::Colors::Black);
}

} // namespace nccu

#endif // QUEST_GIVER_INDICATOR_H_
