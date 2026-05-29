#ifndef QUEST_HOOK_TABLE_H_
#define QUEST_HOOK_TABLE_H_
#include "game/state/SemesterState.h"
#include <functional>
#include <string_view>
#include <vector>

class Player;
class EventBus;     // 鉤子可能發布事件——bus 經參數一路串接傳入

namespace nccu {

/**
 * @file QuestHookTable.h
 * @brief 第一至四章「按 E 互動」任務鉤子的資料化表格與執行入口。
 *
 * 把原本內聯在 GameController::Update 裡約 14 個寫死的 TryXxx(npcId, state)
 * 呼叫，改以一張表登記（消除「每新增型別就得改現有 switch」的開放封閉壞味道）。
 * 當玩家對非閒聊 NPC 按下 E 的當下，會「依登記順序」逐一執行每個鉤子，且發生在
 * 開啟對話框「之前」。每個鉤子都自我閘控於 (state, npcId)，在其章節／NPC 之外
 * 是廉價的 no-op，因此每次互動都跑整張表既正確又順序穩定。
 *
 * 新增章節／NPC 從此是「資料」（在 QuestHookTable.cpp 加一行登記），而非去動
 * 控制器的萬能方法。登記順序具關鍵作用（完全對齊原本的內聯呼叫序列，使自我閘控
 * 語意與任何跨鉤子順序——例如某鉤子設下、後續鉤子讀取的圖書館員相遇旗標——逐位元
 * 保持一致）；RunInteractHooks 由前往後走訪整張表。
 *
 * 純模型邏輯：鉤子會改寫 Player（並讀取 FSM 狀態）。不含 raylib、輸入或渲染——
 * 那些留在控制器／View。
 */

/**
 * @brief 統一的鉤子簽章。
 *
 * 底層鉤子原本有三種形狀：
 *   - f(player, npcId, state)        ——最常見的情形
 *   - f(player, npcId, state, ret)   ——TryReturnLibrarianUmbrella 需要插曲段的
 *                                      回傳目標，以將自身限定在第二章往第三章的市集
 *   - f(player, state)               ——TryApplyCh3Ripple 與 npcId 無關
 * 全部在此轉接成「同一個」參數個數，使整張表同質；轉接 lambda 會丟棄該鉤子用不到
 * 的參數。`returnTo` 即 World::Semester().InterludeReturnTo()，每次呼叫都轉發。
 * 第一個參數固定為 `bus`，使會發布事件的鉤子（TryReturnVictimUmbrella /
 * TryRescueBookworm / TryReturnLibrarianUmbrella / TryAdvanceCh3Trade /
 * TryApplyCh3Ripple）能將 ShowMessage 經注入的 bus 送出，而非走全域 Instance()；
 * 不發布的鉤子忽略它（其轉接 lambda 丟棄此參數）。
 */
using QuestHookFn = std::function<void(EventBus& bus,
                                       Player& player,
                                       std::string_view npcId,
                                       SemesterState state,
                                       SemesterState returnTo)>;

/** @brief 一筆登記的任務鉤子：名稱（供測試／日後記錄）與其函式。 */
struct QuestHook {
    std::string_view name;   ///< 供測試／日後記錄；不影響行為
    QuestHookFn       fn;     ///< 此鉤子的執行函式
};

/**
 * @brief 取得「只建構一次」的有序鉤子表。
 * @return 依登記順序排列的鉤子表（唯讀參照）。
 *
 * 順序完全等同原本內聯的按 E 互動序列：
 *   TryReturnVictimUmbrella、TryRescueBookworm、TryMeetLibrarian、
 *   TryLendLibrarianUmbrella、TryReturnLibrarianUmbrella、TryApplyCh2Ripple、
 *   TryAdvanceCh3Trade、TryApplyCh3Ripple、TryApplyCh4Ripple。
 * （OpenNpcDialog 不是鉤子——它在整張表跑完「之後」才開啟對話框，故仍留在控制器
 * 的互動分派中。）
 */
[[nodiscard]] const std::vector<QuestHook>& InteractQuestHooks();

/**
 * @brief 依登記順序執行所有鉤子。
 * @param bus      事件匯流排，供會發布事件的鉤子送出訊息。
 * @param player   當前玩家（鉤子會讀寫其狀態）。
 * @param npcId    被互動 NPC 的識別字串。
 * @param state    當前學期 FSM 狀態。
 * @param returnTo 插曲段結束後的回傳目標狀態。
 *
 * 每個鉤子內部自我閘控，故只有 (state, npcId) 相符者才會實際動作。設為自由函式，
 * 讓測試能在不經控制器的情況下，對 Player 直接驅動整張表。
 */
void RunInteractHooks(EventBus& bus, Player& player, std::string_view npcId,
                      SemesterState state, SemesterState returnTo);

} // namespace nccu

#endif // QUEST_HOOK_TABLE_H_
