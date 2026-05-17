#ifndef CHAPTER4_QUEST_H_
#define CHAPTER4_QUEST_H_
#include "SemesterState.h"
#include <string_view>

class Player;

namespace nccu {

// Ch4 期末考終焉 peak-ripple once-keys. chapter4.md karma is `- ` bullet
// -doc (not `>` blockquote) → NOTHING is parser-applied; every Ch4
// karma is path-b. These land the reactive ripple karma the (a)/(b)/(c)
// routing (S5e-2a, line-only) cannot, once per Ch4 per effect, so the
// virtuous path can actually clear karma>80 for Ending A and the
// ProfessorTrap arc cashes in its final -15.
inline constexpr const char* kFlagCh4RippledSenior   = "Flag_Ch4Rippled_Senior";
inline constexpr const char* kFlagCh4RippledBookworm = "Flag_Ch4Rippled_Bookworm";
inline constexpr const char* kFlagCh4RippledTAHelped = "Flag_Ch4Rippled_TAHelped";
inline constexpr const char* kFlagCh4RippledProfTrap = "Flag_Ch4Rippled_ProfTrap";

// E-interact hook, sibling of TryApplyCh2Ripple / TryApplyCh3Ripple.
// Per-NPC, once each (independent keys):
//   西裝學長  HelpedSenior && karma>70 → +10  (chapter4.md L109 (b))
//   學霸      BookwormRecovered        → +5   (L168 (b))
//   助教      HelpedTA_Ch1             → +10  (L228 (b) 坦白)
//   助教      HasProfessorTrap         → -15  (L242 (c) 對峙) — lands
//             INDEPENDENTLY even when (b) is shown (L235「(b) 優先，
//             但 (c) 的 karma 扣點仍計算」), separate key.
// The 助教 (d) 體諒/質問 ±15/-5 is the S5e-2d choice, not here. No-op
// outside Ch4 / wrong npc / unmet flag.
void TryApplyCh4Ripple(Player& player, std::string_view npcId,
                       SemesterState state);

} // namespace nccu

#endif // CHAPTER4_QUEST_H_
