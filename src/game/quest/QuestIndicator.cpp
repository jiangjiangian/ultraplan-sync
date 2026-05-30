#include "game/quest/QuestIndicator.h"
#include "game/quest/Chapter1Quest.h"
#include "game/quest/Chapter2Quest.h"
#include "game/quest/Chapter3Quest.h"
#include "game/quest/Chapter4Quest.h"

/**
 * @file QuestIndicator.cpp
 * @brief 依當前章節把任務 `!` 的顯示判定分派給各章節規則的實作。
 */

namespace nccu {

bool QuestIndicatorVisible(std::string_view npcId, bool isQuestGiver,
                           SemesterState state, const Player& player) {
    switch (state) {
        case SemesterState::Chapter1_AddDrop:
            // Ch1 主線為 苦主 → 西裝學長 → 苦主 的三段序列。西裝學長在預設
            // NPC 生成清單中 isQuestGiver=false，因此與 Ch2 學霸、Ch4 助教
            // 同理：此關卡對主線 NPC「不」與 isQuestGiver 做 AND；它把該旗標
            // 傳入，由 Ch1IndicatorVisible 只套用在非主線 NPC 上（使未來新增的
            // Ch1 任務給予者仍依其名冊旗標，而 isQuestGiver=false 的助教／阿姨／
            // 學霸 永遠不會冒充成主線 `!`）。
            return Ch1IndicatorVisible(npcId, isQuestGiver, player);
        case SemesterState::Chapter2_Midterms:
            // Ch2 主線 圖書館管理員 → 學霸 在此排序。不與 isQuestGiver 做 AND
            // ——學霸在 Ch2 名冊中 isQuestGiver=false，卻必須在被喚醒後亮起，
            // 故 Ch2IndicatorVisible 把該旗標當參數傳入，只套用在非主線 NPC
            // 上（與 Ch4 以 npcId 為鍵的關卡相同）。
            return Ch2IndicatorVisible(npcId, isQuestGiver, player);
        case SemesterState::Chapter3_SportsDay:
            // 鏈上 3 個節點是 Ch3 唯一的任務給予者；此關卡依序點亮其 `!`。
            // 非鏈上的 Ch3 任務給予者（目前沒有）會落入 Ch3IndicatorVisible
            // 的 return true，並仍透過下方的 && 尊重 isQuestGiver。
            return isQuestGiver && Ch3IndicatorVisible(npcId, player);
        case SemesterState::Chapter4_Finals:
            // 終局 `!` 以 npcId 為鍵，而非名冊旗標（每個 Ch4 NPC 都
            // isQuestGiver=false）：僅助教，直到玩家做出選擇為止。
            return Ch4IndicatorVisible(npcId, player);
        default:
            return isQuestGiver;   // Interlude／結局：原樣沿用
    }
}

} // namespace nccu
