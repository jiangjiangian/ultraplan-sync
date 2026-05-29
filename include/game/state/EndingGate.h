#ifndef ENDING_GATE_H_
#define ENDING_GATE_H_

/**
 * @file EndingGate.h
 * @brief 結局閘門：以旗標／業力／持有物決定四種結局並驅動學期狀態機轉移。
 */

class Player;                 // 全域命名空間的模型物件
class EventBus;               // 事件匯流排由外部注入，不在本層取用全域 Instance()
namespace nccu {
class SemesterStateMachine;
class DialogState;

/**
 * @brief 依旗標解析結局並轉移到對應的結局狀態，每個非對話幀輪詢一次。
 * @param bus      轉移時用來發布過場提示的事件匯流排（由呼叫端注入）。
 * @param player   讀取業力與旗標的玩家模型。
 * @param semester 達成條件時對其呼叫 Transition() 的學期狀態機。
 * @param dialog   仍在播放時延後判定、轉移時關閉的對話狀態。
 *
 * 全程僅在 Chapter4_Finals 生效；判定優先序 A→B→D→C，先命中者勝。一旦
 * Flag_TaFinaleChoiceMade 設立，閘門即為「完全」——四者必有其一觸發，不會卡死。
 * 結局來源有二：碰觸雨傘（詛咒傘→B／醜傘→C），或助教終局選擇（體諒→A，風雨同行
 * →D，質問冷淡終局→B）。當對話仍 Active() 時延後判定，讓收尾的自白／終局旁白先
 * 被讀完；轉移當下會關閉該對話。
 */
void CheckEndingGates(EventBus& bus, Player& player,
                      SemesterStateMachine& semester, DialogState& dialog);
}
#endif // ENDING_GATE_H_
