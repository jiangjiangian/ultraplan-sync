#ifndef CHAPTER2_QUEST_H_
#define CHAPTER2_QUEST_H_
#include "game/quest/Flags.h"
#include "game/state/SemesterState.h"
#include <string_view>

class Player;                       // 由救援／結束流程改動其狀態
class EventBus;                     // 事件匯流排由外部注入

namespace nccu {

class DialogState;                  // 結束輪詢以 Active() 查詢

/**
 * @file Chapter2Quest.h
 * @brief Ch2 圖書館期中考任務的旗標使用說明與互動鉤子宣告。
 *
 * kFlag* 常數住在 quest/Flags.h；以下描述 Ch2 的救援／結束／漣漪機制如何串接它
 * 們：
 *   - kFlagFoundNote1/2/3：由筆記 pickup（ChapterQuestItems）設置。
 *   - kFlagBookworm：學霸已被能量飲料喚醒並請玩家撿回散落的筆記，即筆記任務「已
 *     啟動」。它把守筆記生成（World::MaybeSpawnChapter2Notes）、學霸對話分流與救
 *     援第二階段。「只」由 TryRescueBookworm 的喚醒步驟設置，絕不由內容設置。
 *   - kFlagMetLibrarian：硬性把守 Ch2 主線（管理員 → 學霸 → 撿筆記 → 學霸）。在
 *     Ch2 中玩家「第一次」與圖書館管理員對話時設置（TryMeetLibrarian，於她的線
 *     索台詞開啟的 E 互動時刻執行）。它同時把守學霸對話（未設置前 DialogOpener
 *     會把他導向「先去問櫃台的管理員」）與 TryRescueBookworm 的喚醒步驟。具冪等
 *     性（已持有時 SetFlag 為 no-op）；只由 TryMeetLibrarian 設置。
 *   - kFlagLibrarianUmbrella：一次性鎖鍵——圖書館管理員已把折疊傘（管理員的傘）
 *     借給玩家。由 TryLendLibrarianUmbrella 設置，使再次對話「不會」重複授予／疊
 *     加借傘。非結局旗標。
 *   - kFlagLibrarianUmbrellaReturned：一次性鎖鍵——玩家已在 Ch2→Ch3 Interlude 的
 *     中正圖書館前「歸還」管理員的傘（選做的責任感節拍）。由
 *     TryReturnLibrarianUmbrella 設置，使 +10 業力與清除借傘恰好觸發一次；非結局
 *     旗標。
 *   - kFlagCh2RippledSuitSenior / kFlagCh2RippledTA：每 NPC 的「Ch2 漣漪業力已落
 *     地」一次性鎖鍵。漣漪對話本身在再次對話時仍可能重現，但只有業力調整被鎖成
 *     每個 Ch2 恰好觸發一次。
 */

/// Interlude 歸還點標記 NPC 的 npcId（圖書館前一個小 NPC，玩家對它按 E 歸還借
/// 傘）。單一來源，使生成、鉤子與對話分流不致漂移。
inline constexpr const char* kNpcLibrarianReturn = "librarian_return";

/**
 * @brief 三頁學霸筆記是否已全數收齊。
 * @param player 玩家（讀取三個筆記旗標）。
 * @return 三頁皆持有回傳 true。
 *
 * 把守圖書館管理員 (b) 與最終的交換。
 */
[[nodiscard]] bool Chapter2NotesComplete(const Player& player);

/**
 * @brief E 互動鉤子：與學霸對話的雙階段任務機器。
 * @param bus    事件匯流排（每個喚醒／完成／提示節拍由此發布 ShowMessage）。
 * @param player 玩家（喚醒時消耗能量飲料，完成時改動旗標與業力）。
 * @param npcId  互動對象識別字串。
 * @param state  目前章節狀態。
 *
 * 以 kFlagBookworm 為鍵。除非 state==Chapter2_Midterms、npcId=="bookworm" 且學霸
 * 尚未復原，否則為 no-op。
 *
 *   - 第一階段（沉睡，未設喚醒旗標）：學霸癱在羅馬廣場雕像下。計數背包中有能量
 *     飲料時於此「消耗」之 → 設 Flag_Bookworm，學霸醒來並請玩家撿回筆記（此刻筆
 *     記任務啟動，World 隨後生成三頁筆記）。沒有飲料則顯示圖書館地下室自販機
 *     （35 元）的退路提示，不消耗任何東西。
 *   - 第二階段（已喚醒，已設喚醒旗標）：三頁筆記若未齊，顯示提醒台詞；三頁齊備
 *     後觸發交換（筆記 ↔ 玩家的傘）：Flag_BookwormRecovered ＋ 業力 +5（path-b：
 *     (d) 區塊未帶旗標標註，故開場的一次性套用不會替它觸發）。能量飲料在「喚
 *     醒」步驟花掉，不在此處。
 *
 * 刻意「不」設 Flag_Ch2Cleared——那由 LiftChapter2Clear 稍後抬升，好讓 (d) 致謝對
 * 話先可被讀完。
 */
void TryRescueBookworm(EventBus& bus, Player& player,
                       std::string_view npcId, SemesterState state);

/**
 * @brief 延後執行的章節結束輪詢，設定 Flag_Ch2Cleared。
 * @param player 玩家（讀取復原旗標與一次性鎖鍵）。
 * @param state  目前章節狀態。
 * @param dialog 對話狀態（以 Active() 確認 (d) 致謝已關閉）。
 *
 * 唯有學霸已復原「且」沒有對話開啟時才設 Flag_Ch2Cleared——即玩家讀完 (d) 致謝之
 * 後，隨即由 CheckChapterGates 既有的 Ch2 條件式轉場至 Interlude。把結束與救援互
 * 動解耦，避免閘門在 (d) 對話開啟的同一幀就把它關掉（與 Flag_LeaveInterlude 同模
 * 式：先觸發，再以輪詢消費）。
 */
void LiftChapter2Clear(Player& player, SemesterState state,
                       const DialogState& dialog);

/**
 * @brief E 互動鉤子：與圖書館管理員（Ch2 鏈頭）相遇。
 * @param player 玩家（第一次對話時設 kFlagMetLibrarian）。
 * @param npcId  互動對象識別字串。
 * @param state  目前章節狀態。
 *
 * 為 TryRescueBookworm 的姊妹，由 GameController 緊鄰它、且在開場「之前」呼叫。
 * 在 Ch2 中玩家第一次與她對話時設 kFlagMetLibrarian（她的 (a) 線索台詞指向羅馬廣
 * 場／學霸），藉此「解鎖」學霸：未設置前 DialogOpener 會把學霸導向「先去問櫃台
 * 的管理員」，且 TryRescueBookworm 拒絕喚醒他。除非 state==Chapter2_Midterms 且
 * npcId=="librarian" 否則為 no-op。具冪等性（已持有時 SetFlag 為 no-op），再次對
 * 話無害。
 */
void TryMeetLibrarian(Player& player, std::string_view npcId,
                      SemesterState state);

/**
 * @brief E 互動鉤子：圖書館管理員把折疊傘（管理員的傘）借給玩家。
 * @param player 玩家（授予手持借傘並設一次性鎖鍵）。
 * @param npcId  互動對象識別字串。
 * @param state  目前章節狀態。
 *
 * 為 TryRescueBookworm 的姊妹，由 GameController 緊鄰它呼叫。chapter2.md 管理員
 * (b) 已說出交付台詞、補設定指定一把「能擋雨但仍會累積雨量」的借傘——本函式在引
 * 擎中串接該節拍。除非 state==Chapter2_Midterms 且 npcId=="librarian" 否則為
 * no-op。以 Flag_Bookworm 把守（(b) 狀態：玩家已喚醒學霸並折返），使借傘恰在她的
 * (b) 台詞播放時抵達。透過 kFlagLibrarianUmbrella 達成冪等，再次對話絕不疊加雨
 * 傘。
 *
 * 效果：SetHeldUmbrella(HeldUmbrella::Loaner)——玩家現在「手持」管理員的傘（一列
 * 背包傘列，其描述說明撐著時自動減緩雨量），且 HasUmbrella() 轉為 true，使既有的
 * 「戶外＋持傘」ApplyRainSheltered 減緩生效。它「不」設 Flag_HasTrueUmbrella——借
 * 傘明確不是真傘，故它本身永遠無法滿足 Ending A 的持傘條件。
 */
void TryLendLibrarianUmbrella(Player& player, std::string_view npcId,
                              SemesterState state);

/**
 * @brief E 互動鉤子：在 Ch2→Ch3 Interlude 歸還管理員的傘（選做的責任感節拍）。
 * @param bus      事件匯流排（由此發布責任感 +10 致謝或重播 ShowMessage）。
 * @param player   玩家（歸還時 +10 業力並清除借傘狀態與旗標）。
 * @param npcId    互動對象識別字串。
 * @param state    目前章節狀態。
 * @param returnTo World::Semester().InterludeReturnTo()，把本鉤子範圍鎖到 Ch2→Ch3
 *                 市集（== Chapter3_SportsDay）而非其他。
 *
 * 為 TryLendLibrarianUmbrella 的姊妹，由 GameController 緊鄰其他鉤子、且在開場
 * 「之前」呼叫。歸還點標記（kNpcLibrarianReturn）由
 * World::MaybeSpawnInterludeLibrarianReturn「僅」在玩家於回到 Ch3 的市集中仍持借
 * 傘時延後生成於中正圖書館前，故本鉤子可觸發時那些前提已成立；鉤子仍會重新檢查
 * （防禦性，也是唯一改動狀態之處）。
 *
 * 三個分支：
 *   - 情境不符：不在 Interlude、不是 Ch2→Ch3 市集、或不是歸還點 npcId → no-op。
 *   - 已歸還：已設 kFlagLibrarianUmbrellaReturned → 重播一句收尾 ShowMessage（再
 *     次對話友善），不給第二次業力。
 *   - 歸還：仍持借傘（Flag_LibrarianUmbrella ＋ HeldUmbrella::Loaner）→ 業力 +10、
 *     清空手持傘（SetHasUmbrella(false)）與 Flag_LibrarianUmbrella、設已歸還鎖
 *     鍵、發致謝 ShowMessage。「不」授予任何影響結局的傘旗標（借傘從來不是真
 *     傘）。玩家若略過，借傘仍會在進入 Ch3 時自動清除（無業力）——故此為純粹正
 *     向的選做選擇，絕非死鎖。
 */
void TryReturnLibrarianUmbrella(EventBus& bus, Player& player,
                                std::string_view npcId, SemesterState state,
                                SemesterState returnTo);

/**
 * @brief E 互動鉤子：落地對話開場無法給予的 Ch1→Ch2 漣漪業力。
 * @param player 玩家（依旗標施加 ±3／-10 業力，每 NPC 一次性）。
 * @param npcId  互動對象識別字串。
 * @param state  目前章節狀態。
 *
 * 為 TryRescueBookworm 的姊妹。chapter2.md 把西裝學長／助教導向以旗標把守的子狀
 * 態，其業力是開場的一次性套用「不會」給的（它被守在尚未設置的旗標之後，但漣漪
 * 旗標是已持有的 Ch1 旗標——或該條目僅有業力而無旗標）。故文件所載的「-10 在此落
 * 地」／±3 必須在此施加，每個 Ch2 每 NPC 一次（每 NPC 一次性鎖鍵）。對其他任何
 * NPC／狀態／旗標組合皆為 no-op。優先序：
 *   - 助教：HasProfessorTrap（(c)，取代 (a)/(b)，-10）優先於 HelpedTA_Ch1（(b)，
 *     0）。
 *   - 西裝學長：HelpedSenior（(b)，+3）與 ScoldedSenior（(c)，-3）互斥。
 */
void TryApplyCh2Ripple(Player& player, std::string_view npcId,
                       SemesterState state);

/**
 * @brief Ch2 主線的循序任務給予者「!」指示燈閘門，為 Ch3IndicatorVisible 的姊妹。
 * @param npcId        要查詢的 NPC 識別字串。
 * @param isQuestGiver 該 NPC 名冊上的任務給予者旗標。
 * @param player       玩家（讀取喚醒／復原旗標）。
 * @return 該 NPC 此幀是否應顯示「!」。
 *
 * 主線為 圖書館管理員(線索) → 學霸(喚醒) → [撿三頁筆記] → 學霸(換回)。「!」依主
 * 線順序推進：
 *   - 圖書館管理員（librarian）：鏈頭，自進入章節起亮燈，直到學霸被喚醒
 *     （kFlagBookworm）。她給出指向羅馬廣場的線索；學霸醒來後她任務完成、熄燈。
 *   - 學霸（bookworm）：「僅」在他被喚醒後亮燈，並貫穿撿筆記與換回，直到
 *     Flag_BookwormRecovered。喚醒前他是暗的（「!」先引導玩家去找管理員），正如
 *     Ch3 的 B 在 A 完成交易前保持暗的。
 * 純以 npcId 為鍵（「非」名冊位）：Ch2 名冊出貨學霸為 isQuestGiver=false，故
 * QuestIndicatorVisible 必須在「不」AND isQuestGiver 的情況下查詢本函式（與 Ch4
 * 終盤閘門相同），否則學霸永不亮燈。其他 npcId 一律回傳其 isQuestGiver 位，使非
 * 主線 NPC 不受影響。View 在 state==Chapter2_Midterms 時呼叫它。
 */
[[nodiscard]] bool Ch2IndicatorVisible(std::string_view npcId,
                                       bool isQuestGiver,
                                       const Player& player);

} // namespace nccu

#endif // CHAPTER2_QUEST_H_
