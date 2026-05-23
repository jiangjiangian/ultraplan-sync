#ifndef QUEST_INDICATOR_H_
#define QUEST_INDICATOR_H_
#include "state/SemesterState.h"
#include <string_view>

class Player;

namespace nccu {

// Single source of truth for "does this NPC paint a quest `!` this frame?"
// — the View calls it once per world object and stays pure-render (no
// gameplay logic leaks into ui/View.cpp). It folds together every
// per-chapter indicator rule so the decision lives in the quest layer.
// EVERY story chapter now sequences its `!` by main-quest order (T3):
//
//   • Chapter1_AddDrop: single-NPC spine (Ch1IndicatorVisible). The 苦主's
//     `!` rides 承諾 → 找傘 → 歸還 and goes dark once the grant fires
//     (Flag_HasTrueUmbrella). The 助教 申請書 errand is isQuestGiver=false,
//     so it never masquerades as the main objective.
//   • Chapter2_Midterms: 圖書館管理員 → 學霸 spine (Ch2IndicatorVisible).
//     The 管理員 is the chain head (lit until 學霸 woken); the 學霸 then
//     lights through the note hunt + 換回. 學霸 ships isQuestGiver=false,
//     so this gate does NOT AND isQuestGiver for the spine NPCs (it takes
//     the bit as an arg and applies it only to non-spine NPCs).
//   • Chapter3_SportsDay: the sequential 物物交換鏈 gate (Ch3IndicatorVisible)
//     — only the next A→B→C link lights, and A is the chain head from
//     chapter entry (Item 4a). The 5 archetypes are isQuestGiver=false in
//     the Ch3 roster, so they never reach the chain branch and stay dark.
//   • Chapter4_Finals: the finale target gate (Ch4IndicatorVisible) — 助教
//     only, until the (d) 結算 choice is made (Item 1b). The Ch4 roster is
//     all isQuestGiver=false, so without this nothing in Ch4 drew a `!`.
//   • Interlude / endings: pure pass-through of the roster isQuestGiver bit.
//
// Keyed on (npcId, state, player) and isQuestGiver. The matching out-of-
// order redirects (talking to the wrong main NPC) live in the per-chapter
// E-interact hooks (TryReturnVictimUmbrella / TryRescueBookworm /
// TryAdvanceCh3Trade), which ShowMessage a nudge instead of advancing.
[[nodiscard]] bool QuestIndicatorVisible(std::string_view npcId,
                                         bool isQuestGiver,
                                         SemesterState state,
                                         const Player& player);

} // namespace nccu

#endif // QUEST_INDICATOR_H_
