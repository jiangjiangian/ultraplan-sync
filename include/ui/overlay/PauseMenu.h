#ifndef UI_OVERLAY_PAUSE_MENU_H_
#define UI_OVERLAY_PAUSE_MENU_H_

namespace nccu {

class World;
namespace engine::render { class IRenderer; }

/**
 * @file PauseMenu.h
 * @brief 遊戲內暫停選單疊層（自 View::RenderOverlays 抽出）。
 */

/**
 * @brief 繪製暫停選單疊層：全螢幕變暗＋置中面板＋6 列（繼續／說明／減少動畫／
 *        擴大目標／重新開始／離開）＋底部一條鍵盤說明帶。
 * @param screenW framebuffer 寬。
 * @param screenH framebuffer 高（變暗與面板皆據此置中）。
 *
 * 反應式：為 World::MenuOpen＋MenuCursor＋ReducedMotion＋LargeTargets（後二者為
 * 兩個切換列）的純函式。View 不保留 UI 狀態——MenuOpen 為 false 時為空操作，故每
 * 幀呼叫皆安全。純渲染（MVC）：以 const 讀取 World、絕不變更。所有繪製皆經注入的
 * IRenderer 與 TextBuilder，故具決定性、可無頭 spy 測試。
 */
void DrawPauseMenu(nccu::engine::render::IRenderer& r,
                   const World& world,
                   float screenW,
                   float screenH);

}  // namespace nccu

#endif  // UI_OVERLAY_PAUSE_MENU_H_
