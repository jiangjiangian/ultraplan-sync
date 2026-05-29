#ifndef UI_OVERLAY_HELP_OVERLAY_H_
#define UI_OVERLAY_HELP_OVERLAY_H_

namespace nccu {

class World;
namespace engine::render { class IRenderer; }

/**
 * @file HelpOverlay.h
 * @brief 遊戲內「說明」（玩法）疊層（自 View::RenderOverlays 抽出）。
 */

/**
 * @brief 繪製遊戲內說明疊層：畫在暫停選單「之上」（選單在其後）。
 *
 * 為 World::MenuOpen＋HelpOpen＋HelpPage 的純函式；與標題畫面共用同一份 GameHelp
 * 文字，使兩者永不漂移。內容為全螢幕遮罩＋一頁說明內文＋「M / E 返回選單」標籤。
 * 每幀呼叫皆安全：除非選單「與」說明同時開啟，否則提前返回。純渲染（MVC）：以
 * const 讀取 World、絕不變更。繪製皆經注入的 IRenderer 與標題畫面共用的
 * nccu::ui::DrawHelpPage helper，故此疊層永不會與開場畫面的說明視圖漂移。
 */
void DrawHelpOverlay(nccu::engine::render::IRenderer& r,
                     const World& world,
                     float screenW,
                     float screenH);

}  // namespace nccu

#endif  // UI_OVERLAY_HELP_OVERLAY_H_
