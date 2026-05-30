#ifndef QUEST_INDICATOR_H_
#define QUEST_INDICATOR_H_
#include "game/state/SemesterState.h"
#include <string_view>

class Player;

namespace nccu {

/**
 * @file QuestIndicator.h
 * @brief 「這個 NPC 本幀要不要畫任務 `!`？」的唯一真實來源，把各章節的提示規則
 *        集中在任務層，讓 View 維持純渲染。
 *
 * View 對每個世界物件呼叫一次本函式即可，遊戲邏輯不會洩漏進 ui/View.cpp。每個故事
 * 章節都依主線順序排序其 `!`：
 *
 *   - Chapter1_AddDrop：單一 NPC 主線（Ch1IndicatorVisible）。苦主的 `!` 隨
 *     承諾 → 找傘 → 歸還 推進，於取得真傘後熄滅。助教的申請書支線
 *     isQuestGiver=false，因此不會被誤認為主目標。
 *   - Chapter2_Midterms：圖書館管理員 → 學霸 主線（Ch2IndicatorVisible）。
 *     管理員為鏈頭（亮到學霸被喚醒為止），學霸接著在找筆記與換回的過程中亮起。
 *     學霸的 isQuestGiver=false，故此關卡對主線 NPC「不」與 isQuestGiver 做 AND；
 *     它把該旗標當參數傳入，只套用在非主線 NPC 上。
 *   - Chapter3_SportsDay：依序點亮的物物交換鏈（Ch3IndicatorVisible）——只有
 *     A→B→C 中的下一環會亮，且 A 自章節進入起就是鏈頭。5 個路人原型在 Ch3 名冊中
 *     皆 isQuestGiver=false，因此不會走到鏈判定而保持熄滅。
 *   - Chapter4_Finals：終局目標關卡（Ch4IndicatorVisible）——僅助教，直到玩家做出
 *     結算選擇為止。Ch4 名冊全為 isQuestGiver=false，少了此關卡 Ch4 將沒有任何 `!`。
 *   - Interlude／結局：直接沿用名冊的 isQuestGiver 旗標。
 *
 * 以 (npcId, state, player) 與 isQuestGiver 為鍵。對應的「找錯主線 NPC」轉向提示
 * 不在此處，而在各章節的 E 互動鉤子（TryReturnVictimUmbrella／TryRescueBookworm／
 * TryAdvanceCh3Trade），那些鉤子會 ShowMessage 一句提醒而非推進進度。
 *
 * @param npcId        受測世界物件的 NPC 識別字串。
 * @param isQuestGiver 名冊標記的任務給予者旗標（部分章節僅套用在非主線 NPC）。
 * @param state        目前學期／章節狀態，決定採用哪條章節規則。
 * @param player       玩家狀態，供章節規則查詢進度旗標。
 * @return 本幀該 NPC 應顯示任務 `!` 時回傳 true。
 */
[[nodiscard]] bool QuestIndicatorVisible(std::string_view npcId,
                                         bool isQuestGiver,
                                         SemesterState state,
                                         const Player& player);

} // namespace nccu

#endif // QUEST_INDICATOR_H_
