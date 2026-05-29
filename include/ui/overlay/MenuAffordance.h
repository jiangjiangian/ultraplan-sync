#ifndef UI_OVERLAY_MENU_AFFORDANCE_H_
#define UI_OVERLAY_MENU_AFFORDANCE_H_

namespace nccu {

class World;
namespace engine::render { class IRenderer; }

/**
 * @file MenuAffordance.h
 * @brief 右上角「M 選單」操作提示（自 View::RenderOverlays 抽出）。
 */

/**
 * @brief 繪製右上角「M 選單」提示：一個小而面板襯底的標籤，恆告訴玩家如何喚出
 *        暫停選單。
 * @param screenW framebuffer 寬。
 * @param screenH framebuffer 高（未使用；標籤錨定右上角，內縮 6px）。
 *
 * 反應式：為 World::MenuOpen 的純函式——選單本身開啟時隱藏此標籤（由選單取代）。
 * 每幀呼叫皆安全；MenuOpen 為真時提前返回。不保留 UI 狀態。純渲染（MVC）：以
 * const 讀取 World、絕不變更。所有繪製皆經注入的 IRenderer 與 TextBuilder，故具
 * 決定性、可無頭 spy 測試。
 */
void DrawMenuAffordance(nccu::engine::render::IRenderer& r,
                        const World& world,
                        float screenW,
                        float screenH);

}  // namespace nccu

#endif  // UI_OVERLAY_MENU_AFFORDANCE_H_
