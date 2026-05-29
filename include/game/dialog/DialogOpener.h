#ifndef DIALOG_OPENER_H_
#define DIALOG_OPENER_H_
#include "game/state/SemesterState.h"
#include <string_view>

class Player;  // 全域命名空間的模型物件——前向宣告以免拉進整個 Player 標頭

namespace nccu {

class DialogState;

/**
 * @file DialogOpener.h
 * @brief 把 (npcId, SemesterState[, Player]) 對映到該開啟哪段對白／選單的開場路由層。
 *
 * 連結 DialogSource 的解析資料與 DialogState 的執行期會話：依任務旗標與業力決定 NPC
 * 該說哪一個子段，並在 NPC 於此狀態分支時組出選單。
 */

/**
 * @brief 純台詞開場：以 (npcId, state, subState) 子段的台詞為對白內容，不帶選項。
 * @param dlg      要開啟的對話狀態。
 * @param npcId    NPC 識別字串。
 * @param state    當前學期狀態。
 * @param subState 指定的子段索引。
 *
 * 找不到對應子段時 dlg 維持非作用中（DialogState::Open({}) 是已載明的 no-op）。
 */
void OpenNpcDialogSub(DialogState& dlg, std::string_view npcId,
                      SemesterState state, int subState);

/**
 * @brief 智慧開場：組出 subState 0 的開場台詞，並為在此狀態分支的 NPC 加上選單。
 * @param dlg   要開啟的對話狀態。
 * @param npcId NPC 識別字串。
 * @param state 當前學期狀態。
 *
 * 分支 NPC 的每個同層 subState >=1 都產生一個 DialogChoice（label = choiceLabel，
 * karma/flag 取自該子段，nextLines = 該子段台詞）；不分支的 NPC 維持純台詞。
 * 無 subState 0 子段時 dlg 維持非作用中。
 */
void OpenNpcDialog(DialogState& dlg, std::string_view npcId,
                   SemesterState state);

/**
 * @brief 依任務旗標決定 (npcId, state) 的開場 subState。
 * @return 路由後的子段索引；0 代表走 OpenNpcDialog 的 (a) 基準開場。
 *
 * 例如 Ch1：ta 在 Flag_FoundForm（取得申請書）後 →1，並在 Flag_HelpedTA_Ch1 後維持 1；
 * victim 在 Flag_PromisedVictim 後 →1（recap，不再選擇）；其餘 →0。純函式，無副作用。
 */
int ResolveOpenerSubState(std::string_view npcId, SemesterState state,
                          const Player& player);

/**
 * @brief 具 Player 感知的開場進入點（GameController 實際呼叫的版本）。
 * @param dlg    要開啟的對話狀態。
 * @param player 當前玩家（讀旗標／業力，並可能被施加一次性獎勵）。
 * @param npcId  NPC 識別字串。
 * @param state  當前學期狀態。
 *
 * 先解析開場 subState：為 0 時委派給三參數版 OpenNpcDialog（保留 1b-2 選單路徑）；
 * >=1 時以純台詞開啟該子段，並「僅一次」施加其 karma/flag（防護：只在該子段要設定一個
 * 玩家尚未持有的 true 旗標時才套用）。
 */
void OpenNpcDialog(DialogState& dlg, Player& player,
                   std::string_view npcId, SemesterState state);

} // namespace nccu
#endif // DIALOG_OPENER_H_
