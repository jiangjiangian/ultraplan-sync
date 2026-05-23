#ifndef QUEST_INDICATOR_H_
#define QUEST_INDICATOR_H_
#include "state/SemesterState.h"
#include <string_view>

class Player;

namespace nccu {

// Single source of truth for "does this NPC paint a quest `!` this frame?"
// — the View calls it once per world object and stays pure-render (no
// gameplay logic leaks into ui/View.cpp). It folds together every
// per-chapter indicator rule so the decision lives in the quest layer:
//
//   • Default (Ch1 / Ch2 / Interlude / endings): the roster's isQuestGiver
//     bit, unchanged — e.g. Ch1 苦主 and Ch2 管理員 keep their lone `!`.
//   • Chapter3_SportsDay: the sequential 物物交換鏈 gate (Ch3IndicatorVisible)
//     — only the next A→B→C link lights, and A is the chain head from
//     chapter entry (Item 4a). The 5 archetypes are isQuestGiver=false in
//     the Ch3 roster, so they never reach the chain branch and stay dark
//     (no stray `!` — Item 4b is structurally a no-op, the roster already
//     marks only the 3 chain nodes as quest-givers).
//   • Chapter4_Finals: the finale target gate (Ch4IndicatorVisible) — 助教
//     only, until the (d) 結算 choice is made (Item 1b). The Ch4 roster is
//     all isQuestGiver=false, so without this nothing in Ch4 drew a `!`.
//
// Keyed on (npcId, state, player) and isQuestGiver, so it overrides the
// roster bit ONLY where a chapter needs it (Ch3 chain, Ch4 finale) and is a
// pure pass-through everywhere else.
[[nodiscard]] bool QuestIndicatorVisible(std::string_view npcId,
                                         bool isQuestGiver,
                                         SemesterState state,
                                         const Player& player);

} // namespace nccu

#endif // QUEST_INDICATOR_H_
