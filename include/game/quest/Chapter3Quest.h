#ifndef CHAPTER3_QUEST_H_
#define CHAPTER3_QUEST_H_
#include "game/quest/Flags.h"
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"
#include <string_view>

class Player;                       // 由交換鉤子改動其狀態
class EventBus;                     // 事件匯流排由外部注入

namespace nccu {

/**
 * @file Chapter3Quest.h
 * @brief Ch3 校慶運動會物物交換鏈任務的旗標使用說明、場地幾何常數與鉤子宣告。
 *
 * kFlag* 常數住在 quest/Flags.h。一次性任務物品（香腸／大聲公）以「旗標」建模
 * （kFlagHasSausage / kFlagHasLoudspeaker / kFlagKnowsUmbrellaLoc），而非計數消
 * 耗品背包——沿用 Flag_FoundNote / Flag_FoundForm 慣例。kFlagSportsLapDone 在玩家
 * 繞操場跑完一圈（參加校慶）後設置，藉以解鎖 A→B→C 物物交換鏈（走完操場 → 觸發
 * 找 ABC）。kFlagCh3RippledProfTrap 與 kFlagCh2RippledTA 是「分開」的鍵，使 Ch3
 * 的 -10 即便 Ch2 已扣過仍獨立計算一次（chapter3.md：助教旗標若已在 Ch2 扣過，本
 * 次 -10 獨立計算、不重複）。
 */

/// Flag_KnowsUmbrellaLoc 設置後 Ch3 真傘出現的位置
/// （World::MaybeSpawnChapter3Umbrella）。位於體育館「左側」、風雩樓與體育館之間
/// 的開闊缺口，使它「可見且可達」，而非像舊的 (1640,375) 那樣被遮在體育館範圍
/// 內。已遮罩驗證 box 可走且能自操場中心 flood 抵達。Ch4 那把藏在體育館後方的傘
/// 與此無關，仍在 (1640,375)。
inline constexpr nccu::engine::math::Vec2 kChapter3UmbrellaPos{1320.0f, 520.0f};

/// @name 操場校慶跑道幾何
/// 人群跑者（World 生成）、跑圈進度追蹤（World::UpdateSportsLap）與跑道環繪製
/// （View）的單一事實來源。跑道為「田徑場（stadium）」形狀：上下兩段半長
/// kSportsTrackHalfLen 的直線，由左右兩個半徑 kSportsTrackR 的半圓接起，以操場為
/// 中心。(Cx,Cy) 是場地中心；跑圈進度角仍繞它量測。
///@{
inline constexpr float kSportsTrackCx      = 1694.0f;   ///< 操場中心 x
inline constexpr float kSportsTrackCy      = 740.0f;    ///< 操場中心 y
inline constexpr float kSportsTrackHalfLen = 150.0f;    ///< 直線段半長（a）
inline constexpr float kSportsTrackR       = 130.0f;    ///< 端帽半徑（r）
///@}

/**
 * @brief E 互動鉤子：依序推進 chapter3.md 物物交換鏈，每次對話前進一環。
 * @param bus    事件匯流排（由此發布拿到香腸／大聲公／位置的 ShowMessage）。
 * @param player 玩家（每環施加業力並切換鏈旗標）。
 * @param npcId  互動對象識別字串。
 * @param state  目前章節狀態。
 *
 * 三環：
 *   - A 系烤香腸攤主 → Flag_HasSausage，業力 +3（鏈起點）。
 *   - B 系大聲公持有者 → 清香腸＋設 Loudspeaker，業力 +3（第二環）。
 *   - C 系學姊 → 清大聲公＋設 KnowsLoc，業力 +5（情報揭露）。
 * chapter3.md (b)「交易完成」區塊帶業力卻「無」旗標標註，故開場的一次性套用不會
 * 給予——path-b，在此施加。每環的守衛使其恰好觸發一次（它所設的鏈旗標會在再次對
 * 話時把它擋掉）。對其他任何 NPC／狀態／鏈位置皆為 no-op。道具箱裡的真傘才是真正
 * 的章節結束（與 Ch1 同構：claim → UmbrellaClaimed → EventWiring Ch3 條件式 →
 * Interlude returnTo Ch4）；交換鏈是業力／敘事路徑，而非硬性閘門（與 Ch1 的選做
 * 任務相同）。
 */
void TryAdvanceCh3Trade(EventBus& bus, Player& player, std::string_view npcId,
                        SemesterState state);

/**
 * @brief A→B→C 交換鏈的循序任務給予者「!」閘門。
 * @param npcId  要查詢的 NPC 識別字串。
 * @param player 玩家（讀取鏈旗標）。
 * @return 只有「下一個」該對話的環回傳 true（A 完成交易前是 A，再來 B，再來
 *         C）；非鏈 NPC 一律回傳 true。
 *
 * 使三盞指示燈依序揭露，而非一次全亮。View 在 state==Chapter3_SportsDay 時呼叫
 * 它。
 */
[[nodiscard]] bool Ch3IndicatorVisible(std::string_view npcId,
                                       const Player& player);

/**
 * @brief E 互動鉤子：落地 Ch3 的 ProfessorTrap 漣漪，為 TryApplyCh2Ripple 的姊妹。
 * @param bus    事件匯流排（由此發布 proftrap -10 的 ShowMessage）。
 * @param player 玩家（持陷阱傘時 -10 業力，每 Ch3 一次性）。
 * @param state  目前章節狀態。
 *
 * 對應 chapter3.md 章節結尾分支二（業力 -10，Flag_HasProfessorTrap 回呼「Ch1 漣
 * 漪延伸至 Ch3」）。chapter3.md 以項目符號記錄業力（非區塊引用），故 Ch3 沒有任
 * 何業力是解析器套用的——每筆 Ch3 業力皆 path-b。敘事觸發是「持 ProfessorTrap 進
 * 入體育館後台，後台某個同學認出」；因無後台 NPC 段落，故泛化為「第一個 Ch3 NPC
 * 注意到」，透過 kFlagCh3RippledProfTrap 每 Ch3 套用一次（與 Ch2 的鍵獨立——該
 * -10 明確不重複、但每章「分開」計）。在 Ch3 之外／未持旗標時為 no-op。
 */
void TryApplyCh3Ripple(EventBus& bus, Player& player, SemesterState state);

} // namespace nccu

#endif // CHAPTER3_QUEST_H_
