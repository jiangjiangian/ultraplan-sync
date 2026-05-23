#include "quest/QuestIndicator.h"
#include "quest/Chapter1Quest.h"
#include "quest/Chapter2Quest.h"
#include "quest/Chapter3Quest.h"
#include "quest/Chapter4Quest.h"

namespace nccu {

bool QuestIndicatorVisible(std::string_view npcId, bool isQuestGiver,
                           SemesterState state, const Player& player) {
    switch (state) {
        case SemesterState::Chapter1_AddDrop:
            // T3: the Ch1 main spine is single-NPC (苦主). The `!` rides him
            // until the grant (Flag_HasTrueUmbrella), then goes dark. The
            // 苦主 IS a quest-giver, so AND with isQuestGiver (the side 助教
            // 申請書 errand is isQuestGiver=false → never a main `!`).
            return isQuestGiver && Ch1IndicatorVisible(npcId, player);
        case SemesterState::Chapter2_Midterms:
            // T3: the Ch2 main spine 圖書館管理員 → 學霸 sequences here. NOT
            // AND-ed with isQuestGiver — 學霸 ships isQuestGiver=false in the
            // Ch2 roster yet must light once woken, so Ch2IndicatorVisible
            // takes the bit as an argument and applies it only to non-spine
            // NPCs (mirrors the Ch4 npcId-keyed gate).
            return Ch2IndicatorVisible(npcId, isQuestGiver, player);
        case SemesterState::Chapter3_SportsDay:
            // The 3 chain nodes are the only Ch3 quest-givers; the gate
            // sequences their `!`. A non-chain Ch3 quest-giver (none today)
            // would fall through Ch3IndicatorVisible's `return true`, still
            // honouring isQuestGiver via the && below.
            return isQuestGiver && Ch3IndicatorVisible(npcId, player);
        case SemesterState::Chapter4_Finals:
            // Finale `!` is keyed on npcId, NOT the roster bit (every Ch4
            // NPC is isQuestGiver=false): 助教 until the choice is made.
            return Ch4IndicatorVisible(npcId, player);
        default:
            return isQuestGiver;   // Interlude / endings: as-is
    }
}

} // namespace nccu
