#ifndef UI_HUD_STATUS_PANEL_H_
#define UI_HUD_STATUS_PANEL_H_

namespace nccu {

class World;
namespace engine::render { class IRenderer; }

/**
 * @file StatusPanel.h
 * @brief 左上角 HUD 狀態面板（自 View::RenderHud 抽出）。
 */

/**
 * @brief 繪製左上角狀態面板：一塊半透明黑底矩形加上最多 7 列文字——WASD 提示、
 *        Tab/M 操作提示、業力＋雨傘、金幣、Inside（身處建築內時）、章節名、雨量
 *        讀數。
 *
 * 純渲染、螢幕座標。寬度由實際 UTF-8 碼點數估算，使襯底貼齊最寬的一列（章節／
 * 金幣為 CJK，故視為 ×2 寬）。所有數值皆讀自傳入的 World（及其 Player）；本函式
 * 絕不變更狀態（MVC——「View = 只渲染」）。不呼叫 raylib；一切皆經注入的
 * IRenderer，故具決定性、可由無頭流程逐位元驗證。錨定在原程式使用的固定
 * (10, 10) 螢幕位移，呼叫端無需調整。
 */
void DrawStatusPanel(nccu::engine::render::IRenderer& r, const World& world);

}  // namespace nccu

#endif  // UI_HUD_STATUS_PANEL_H_
