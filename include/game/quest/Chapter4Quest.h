#ifndef CHAPTER4_QUEST_H_
#define CHAPTER4_QUEST_H_
#include "game/quest/Flags.h"
#include "game/state/SemesterState.h"
#include <string_view>

class Player;

namespace nccu {

class DialogState;   // 由 TryOpenEndingConfession 改動（開啟自白）

/**
 * @file Chapter4Quest.h
 * @brief Ch4 期末考終焉巔峰漣漪的旗標使用說明與終盤鉤子宣告。
 *
 * kFlag* 常數住在 quest/Flags.h。chapter4.md 的業力以項目符號記錄（非區塊引
 * 用），故「沒有」任何業力是解析器套用的——每筆 Ch4 業力皆 path-b。kFlagCh4Rippled*
 * 一次性鎖鍵落地 (a)/(b)/(c) 分流（僅台詞）無法給予的反應式漣漪業力，每 Ch4 每效
 * 果一次，使善行路徑能真正達到 karma>80 而走 Ending A、並讓 ProfessorTrap 線兌現
 * 它最後的 -15。
 *
 * kFlagBoughtCoffeeForAuntie 是 Ch1 漣漪種子：玩家在 Chapter 1 請福利社阿姨喝了
 * 熱咖啡（chapter1.md 福利社阿姨 (d)，一個不平凡的慷慨選擇——與 Flag_HelpedSenior
 * 的「請學長喝熱咖啡」同一 GDD 模型）。經 Ch1 DialogChoice 路徑設置；在 Ch4 由
 * ResolveOpenerSubState（阿姨 (a) 直接情報 vs (d) 間接情報分流）與 TryApplyCh4Ripple
 * （+3 情分回呼）消費。命名取自 GDD 自身的 LLM 輸入結構範例。
 */

/**
 * @brief Ch4 終盤的「!」目標提示。
 * @param npcId  要查詢的 NPC 識別字串。
 * @param player 玩家（讀取結算抉擇鎖鍵）。
 * @return 助教在 (d) 結算抉擇定案（Flag_TaFinaleChoiceMade）之前回傳 true；其他
 *         npcId 一律回傳 false。
 *
 * Ch4 名冊是閘門驅動，每個 NPC 皆出貨 isQuestGiver=false，原本導致終盤章節完全不
 * 畫「!」、玩家沒有視覺線索知道結局在哪裡決定。本斷言是 Ch3IndicatorVisible 的
 * Ch4 對應：使終盤「!」恰好標記唯一推進主線的 NPC，並在抉擇做出後熄滅。View 在
 * state==Chapter4_Finals 時透過 QuestIndicatorVisible 查詢它；因以 npcId 為鍵（非
 * 名冊的 isQuestGiver 位），閘門驅動的名冊維持 byte 不變。
 */
[[nodiscard]] bool Ch4IndicatorVisible(std::string_view npcId,
                                       const Player& player);

/**
 * @brief 溫柔終盤的授傘：當玩家對助教選擇「體諒」時把真傘還給玩家。
 * @param player 玩家（設 Flag_HasTrueUmbrella ＋ HasUmbrella）。
 * @param npcId  互動對象識別字串。
 * @param state  目前章節狀態。
 *
 * 玩家選擇體諒（Flag_ConsoledTA，由該選項的 ApplyDialogChoice 設置）時，助教把玩
 * 家的真傘塞回手中——故溫柔路徑設 Flag_HasTrueUmbrella ＋ HasUmbrella（口白「拿回
 * 你的傘」節拍在選項的 nextLines 內）。此即 Ending A 的持傘條件：在 karma>80 下，
 * 單憑「體諒」即可達 Ending A，與隱藏的 Ch4 傘平行（且不需要它）。強硬的「質問」
 * 分支永不設 Flag_ConsoledTA，故永不抵達此處，會解析為 Ending B（冷淡終盤）。具
 * 冪等性（HasFlag 守衛）。在 Ch4 之外／NPC 不符／體諒之前皆為 no-op。由
 * GameController 在助教終盤抉擇被確認、且 ApplyDialogChoice 已落地 Flag_ConsoledTA
 * 「之後」呼叫。
 */
void TryGrantTaFinaleUmbrella(Player& player, std::string_view npcId,
                              SemesterState state);

/**
 * @brief E 互動鉤子，為 TryApplyCh2Ripple / TryApplyCh3Ripple 的姊妹。
 * @param player 玩家（依旗標施加各筆漣漪業力）。
 * @param npcId  互動對象識別字串。
 * @param state  目前章節狀態。
 *
 * 每 NPC 各自一次（鍵獨立）：
 *   - 西裝學長：HelpedSenior && karma>70 → +10（(b) 崩潰坦白）。
 *   - 學霸：BookwormRecovered → +5（(b) Ch2 救他回呼）。
 *   - 助教：HelpedTA_Ch1 → +10（(b) 坦白）。
 *   - 助教：HasProfessorTrap → -15（(c) 對峙）——即便 (b) 顯示仍「獨立」落地
 *     （(b) 優先，但 (c) 的扣點仍計算），用分開的鍵。
 *   - 福利社阿姨：BoughtCoffeeForAuntie_Ch1 → +3（(a) 直接情報回呼：Ch1 情分兌
 *     現），一次。
 * 助教 (d) 體諒/質問的 ±15/-5 是對話選項處理，不在此處。在 Ch4 之外／NPC 不符／
 * 旗標未滿足時為 no-op。
 */
void TryApplyCh4Ripple(Player& player, std::string_view npcId,
                       SemesterState state);

/**
 * @brief 在每個 Ch4 結局之前插入一段內心獨白（自白），使結局收束連貫而非突兀。
 * @param player 玩家（讀寫各自白一次性鎖鍵）。
 * @param dialog 對話狀態（開啟自白對話）。
 * @param state  目前章節狀態。
 * @return 本次呼叫若開啟了自白（現有對話開啟）則回傳 true。
 *
 * 一次性鎖鍵 kFlagCh4ConfessedCursed / kFlagCh4ConfessedUgly /
 * kFlagCh4ConfessedTrue 是純程式旗標（無內容引用），由本函式設置：每個結局觸發
 * 在閘門收束「之前」恰好播一次簡短自白。
 *
 * 由 GameController 在每個「非對話」幀、且在 CheckEndingGates「之前」輪詢（後者本
 * 身在對話開啟時即早退）。當 Ch4 進入某個決定結局的狀態、且其自白尚未播放時，本
 * 函式開啟一段簡短自白對話並設該路徑的一次性鎖鍵；如今開啟的對話使
 * CheckEndingGates 延後到玩家關閉它後，閘門才觸發。涵蓋三個缺乏自有收尾節拍的 Ch4
 * 結局觸發：
 *   - Flag_TookCursedUmbrella（自 Ch1 帶來）→ Ending B 前的「詛咒纏上」自白。
 *     （冷淡終盤／質問已有自己的 nextLines。）
 *   - Flag_BoughtUglyUmbrella → Ending C 前的「務實」自言自語。
 *   - 從「地上」撿回隱藏的體育館真傘（Flag_HasTrueUmbrella）卻尚未完成助教終盤 →
 *     釋然自白。溫柔的體諒終盤授予同一旗標卻播自己的節拍（選項的 nextLines），故
 *     此處以 !Flag_TaFinaleChoiceMade 把守，避免在那裡雙重旁白。
 *
 * 具冪等性（一次性鎖鍵）；在 Ch4 之外／對話開啟中／對應自白已播放後皆為 no-op。
 * 優先序與閘門一致（詛咒 → B 優先於其他），使自白與真正觸發的結局相符。
 */
bool TryOpenEndingConfession(Player& player, DialogState& dialog,
                             SemesterState state);

} // namespace nccu

#endif // CHAPTER4_QUEST_H_
