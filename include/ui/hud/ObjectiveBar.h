#ifndef UI_HUD_OBJECTIVE_BAR_H_
#define UI_HUD_OBJECTIVE_BAR_H_

#include "game/state/SemesterState.h"

namespace nccu {

class World;
namespace engine::render { class IRenderer; }

/**
 * @file ObjectiveBar.h
 * @brief 任務目標列（自 View::RenderHud 抽出）。
 */

/**
 * @brief 繪製任務目標列：一行面板襯底的提示，置於頂部中央但「位於左側狀態面板
 *        下方」（≤6 列約到 y132），使章節目標永不與業力／金幣／雨量讀數重疊。
 *
 * 反應式：為 `CurrentObjective(st, *world.GetPlayer())` 的純函式，每幀呼叫皆安
 * 全——無 Player 或目標字串為空時即空操作。純渲染（MVC）：以 const 讀取 World、
 * 絕不變更。寬度經 TextBuilder::Measure() 精確量測，使襯底面板隨目標文字伸縮。
 */
void DrawObjectiveBar(nccu::engine::render::IRenderer& r,
                      const World& world,
                      SemesterState st,
                      float screenW,
                      float screenH);

}  // namespace nccu

#endif  // UI_HUD_OBJECTIVE_BAR_H_
