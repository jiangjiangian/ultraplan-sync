#ifndef CHAPTER1_QUEST_H_
#define CHAPTER1_QUEST_H_
#include "game/quest/Flags.h"
#include "game/state/SemesterState.h"
#include <string_view>

class Player;                       // 由歸還／授予流程改動其狀態
class EventBus;                     // 事件匯流排由外部注入

namespace nccu {

class DialogState;                  // LiftChapter1Clear 以 const 讀取

/**
 * @file Chapter1Quest.h
 * @brief Ch1 加退選之亂「善有善報」任務的旗標使用說明與互動鉤子宣告。
 *
 * 實際的 kFlag* 常數住在 quest/Flags.h（每個 Flag_* 字串的單一事實來源）；本檔
 * 記錄 Ch1 如何「使用」它們：
 *   - kFlagPromisedVictim：由 chapter1.md (b) 承諾 DialogChoice 設置（業力 +5，
 *     於 ApplyDialogChoice 套用）。
 *   - kFlagHasVictimUmbrella：由玩家在集英樓附近撿到的 QuestFlagPickup 設置（與
 *     kFlagHasSausage / kFlagFoundForm 同模式：以旗標而非計數消耗品建模的一次性
 *     任務物品），由 TryReturnVictimUmbrella 的授予階段讀取。
 *   - kFlagClearChapter1：一次性鎖鍵——表示 Ch1 結束（發布 UmbrellaClaimed →
 *     進入 Interlude）已觸發；由 LiftChapter1Clear 輪詢，確保在 (d) 交換對話關閉
 *     後恰好發布一次。
 */

/**
 * @brief E 互動鉤子：在 Ch1 把傘歸還給苦主。
 * @param bus    事件匯流排（玩家尚未持傘時由此發布提示 ShowMessage）。
 * @param player 玩家（授予階段會改動其旗標／持傘狀態）。
 * @param npcId  互動對象識別字串。
 * @param state  目前章節狀態。
 *
 * 重新設計後章節的「互惠」核心：唯有玩家把苦主的傘「帶回去」交給他，章節才結
 * 束，撿地上的傘的那一刻並不會結束。為 TryRescueBookworm（Ch2）／
 * TryAdvanceCh3Trade（Ch3）的姊妹函式，由 GameController 的 E 互動緊鄰它們呼
 * 叫。除非 state==Chapter1_AddDrop 且 npcId=="victim"，否則為 no-op。
 *
 * 四個分支：
 *   - 已完成：已持 Flag_HasTrueUmbrella → no-op（授予已發生；再對話走 (d) 回顧）。
 *   - 未承諾：!Flag_PromisedVictim → no-op（承諾由 (a)/(b) 對話選項處理，尚無傘可
 *     還）。
 *   - 已承諾但無傘：以 ShowMessage 提醒「先幫他找回那把傘」，不改狀態（玩家仍須
 *     去撿 pickup）。
 *   - 授予：已承諾且持 Flag_HasVictimUmbrella，即回報——清除
 *     Flag_HasVictimUmbrella（苦主取回他的傘）＋ SetHasUmbrella(true)＋
 *     SetFlag(Flag_HasTrueUmbrella)（Ending A 的精確條件）。此處「不」發布
 *     UmbrellaClaimed——驅動 Ch1→Interlude 轉場的那一步延後到
 *     LiftChapter1Clear，好讓 (d) 重逢致謝交換對話「先」播放。授予具冪等性：它
 *     不會清除任何自己又會重讀的東西，且上面的「已完成」守衛阻止二次授予。
 *
 * 重新設計後場景上的 TrueUmbrella 已移除（改由苦主授予）；Cursed／Fragile／
 * ProfessorTrap 三把傘仍留在地上作為道德／Ending-B 路徑，並透過各自的 BeClaimed
 * 結束 Ch1——本鉤子是「唯一」的非詛咒結束路徑。
 */
void TryReturnVictimUmbrella(EventBus& bus, Player& player,
                             std::string_view npcId, SemesterState state);

/**
 * @brief 延後執行的 Ch1 章節結束輪詢，為 Chapter2Quest::LiftChapter2Clear 的姊妹。
 * @param bus    事件匯流排（由此發布 ShowMessage + UmbrellaClaimed）。
 * @param player 玩家（讀取授予旗標與一次性鎖鍵）。
 * @param state  目前章節狀態。
 * @param dialog 對話狀態（以 dialog.Active() 確認 (d) 對話已關閉）。
 *
 * 一旦 TryReturnVictimUmbrella 授予了 Flag_HasTrueUmbrella，本函式才觸發章節結
 * 束——但僅在苦主的 (d) 重逢致謝交換對話「關閉之後」，好讓玩家先讀完交換場景，
 * Ch1 才切到 Interlude。發布順序為 ShowMessage 再 UmbrellaClaimed("TrueUmbrella")
 * （與 BeClaimed 的 HUD 槽位順序一致），隨後由 EventWiring 的 Ch1 條件式轉場
 * Ch1→Interlude（returnTo Ch2）。以 kFlagClearChapter1 上鎖，雖每個非對話幀都輪
 * 詢卻恰好觸發一次。在 Ch1 之外／授予之前／對話仍開啟時／已觸發後皆為 no-op。由
 * GameController 緊鄰 LiftChapter2Clear、且在 CheckChapterGates「之前」呼叫，使發
 * 布的轉場同幀即被觀察到。
 */
void LiftChapter1Clear(EventBus& bus, Player& player, SemesterState state,
                       const DialogState& dialog);

/**
 * @brief Ch1 主線的循序「!」指示燈閘門。
 * @param npcId        要查詢的 NPC 識別字串。
 * @param isQuestGiver 該 NPC 名冊上的任務給予者旗標。
 * @param player       玩家（讀取 Ch1 旗標決定走到第幾步）。
 * @return 該 NPC 此幀是否應顯示「!」。
 *
 * 為三步序列 苦主 → 西裝學長 → 苦主（西裝學長作為「中間」一步亮燈）。沿用
 * Ch2/Ch4 慣例（吃進名冊的 isQuestGiver 位，而非由呼叫端先 AND 起來），因為西裝
 * 學長在 DefaultNpcSpawns 出貨為 isQuestGiver=false，卻「必須」在第二步亮燈。序
 * 列以既有 Ch1 旗標為鍵：
 *   - 第 1 步 —!Flag_PromisedVictim：苦主亮（給出線索）。
 *   - 第 2 步 — Flag_PromisedVictim && !Flag_SuitSeniorChoiceMade：西裝學長亮
 *     （對峙／抉擇）。
 *   - 第 3 步 — Flag_SuitSeniorChoiceMade && !Flag_HasTrueUmbrella：苦主再次亮
 *     （歸還他的傘）。
 *   - 完成 — Flag_HasTrueUmbrella：全暗（授予完成；(d) 重逢是回顧）。
 * 只有主線 NPC（victim / suit_senior）在此被排序；其他 NPC 一律沿用名冊的
 * isQuestGiver 位（助教申請書支線／學霸／阿姨皆 isQuestGiver=false，永不充當主目
 * 標）。越序「接觸」由 E 互動鉤子（TryReturnVictimUmbrella 的 ShowMessage 提醒）
 * 重導，不在此處——本函式只負責畫「!」。View 在 state==Chapter1_AddDrop 時透過
 * QuestIndicatorVisible 呼叫它。
 */
[[nodiscard]] bool Ch1IndicatorVisible(std::string_view npcId,
                                       bool isQuestGiver,
                                       const Player& player);

/// Ch1 樂活小舖醜綠傘的售價（阿姨在 chapter1.md 報的八十塊）。單一來源，使提示
/// 文字與扣款不致漂移。
inline constexpr int kCh1UglyUmbrellaPrice = 80;

/**
 * @brief E 互動／對話確認鉤子：在 Ch1 向阿姨購買醜綠傘。
 * @param bus         事件匯流排（由此發布 花費/餘額 或 你錢不夠 提示）。
 * @param player      玩家（成功時扣款並授予手持醜傘）。
 * @param npcId       互動對象識別字串。
 * @param choiceLabel 玩家所選的對話選項標籤。
 * @param state       目前章節狀態。
 * @return 唯有真正完成購買才回傳 true；任何 no-op／放棄／錢不夠路徑皆回傳 false。
 *
 * 此買賣是一筆真實交易：扣除 kCh1UglyUmbrellaPrice 並授予「手持」醜傘
 * （SetHeldUmbrella(HeldUmbrella::Ugly) → 一列背包列＋自動擋雨），並顯示
 * 花費/餘額提示（沿用 Vendor::TryBuy 慣例與同一份 vendor::msg 文案）。由
 * GameController 在阿姨的「購買醜綠傘」選項被確認時呼叫。除非
 * state==Chapter1_AddDrop 且 npcId=="shop_auntie" 且所選標籤正是購買標籤，否則為
 * no-op。
 *
 * 三個分支：
 *   - 已擁有：已持 HeldUmbrella::Ugly → no-op（冪等；阿姨的選單會重新呈現，若無
 *     此守衛，再選一次會為手上已有的傘再扣 80）。
 *   - 錢不夠：DeductMoney(price) 失敗 → ShowMessage「你錢不夠」，錢包與手持傘皆
 *     不動（購買失敗）。
 *   - 購買：金額足夠 → 扣款、SetHeldUmbrella(Ugly)、花費/餘額提示。「關鍵」在於
 *     它「不」設 Flag_BoughtUglyUmbrella——那是 Ch4 集英樓 Vendor 對 Ending-C 的
 *     承諾，而非這把章內的傘。無業力。
 */
bool TryBuyAuntieUglyUmbrella(EventBus& bus, Player& player,
                              std::string_view npcId,
                              std::string_view choiceLabel,
                              SemesterState state);

} // namespace nccu

#endif // CHAPTER1_QUEST_H_
