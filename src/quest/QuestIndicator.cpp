#include "quest/QuestIndicator.h"
#include "quest/Chapter3Quest.h"
#include "quest/Chapter4Quest.h"

namespace nccu {

bool QuestIndicatorVisible(std::string_view npcId, bool isQuestGiver,
                           SemesterState state, const Player& player) {
    switch (state) {
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
            return isQuestGiver;   // Ch1 / Ch2 / Interlude / endings: as-is
    }
}

} // namespace nccu
