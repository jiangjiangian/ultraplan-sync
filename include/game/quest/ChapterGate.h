#ifndef CHAPTER_GATE_H_
#define CHAPTER_GATE_H_
class Player;                 // 全域命名空間的模型物件
class EventBus;               // 事件匯流排由外部注入，非 Instance()
namespace nccu {
class SemesterStateMachine;
class DialogState;

/**
 * @file ChapterGate.h
 * @brief 旗標驅動的「章節 ↔ Interlude」推進主軸。
 */

/**
 * @brief 在套用對話選項後檢查並執行章節 ↔ Interlude 轉場。
 * @param bus      事件匯流排（轉場提示由此發布，使用呼叫端的匯流排實例）。
 * @param player   玩家（讀取並消費推進旗標）。
 * @param semester 學期狀態機（執行 Transition／記錄 returnTo）。
 * @param dialog   對話狀態（轉場時 Close() 掉開啟中的對話）。
 *
 * 為 CheckEndingGates 的姊妹（同地點、同引數），建構可再進入的轉場核心，使整個遊
 * 戲可走通 Ch1 → 市 → Ch2 → 市 → Ch3 → 市 → Ch4：
 *   - Chapter2_Midterms + Flag_Ch2Cleared → Interlude（returnTo Ch3）
 *   - Chapter3_SportsDay + Flag_Ch3Cleared → Interlude（returnTo Ch4）
 *   - Interlude_Market + Flag_LeaveInterlude → 已記錄的 returnTo
 * 採並列 if 形狀，與 CheckEndingGates／1c UmbrellaClaimed 閘門相同；未來新增閘門
 * 即在此加並列 if，勿泛化。Ch1 → Interlude 的進入仍留在 EventWiring 的
 * UmbrellaClaimed 訂閱者（它同時播下 returnTo = Chapter2_Midterms），在此重複會雙
 * 重觸發。Ch4 → 結局是 EndingGate 的職責，不在此處。任何轉場時皆 Close() 掉開啟中
 * 的對話，避免殘留的 Active 對話在下一章背後繼續吃輸入。
 */
void CheckChapterGates(EventBus& bus, Player& player,
                       SemesterStateMachine& semester, DialogState& dialog);
}
#endif // CHAPTER_GATE_H_
