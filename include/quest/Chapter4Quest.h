#ifndef CHAPTER4_QUEST_H_
#define CHAPTER4_QUEST_H_
#include "state/SemesterState.h"
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
inline constexpr const char* kFlagCh4RippledAuntie   = "Flag_Ch4Rippled_Auntie";

// Ch1 漣漪種子: the player bought 福利社阿姨 a hot coffee in Chapter 1
// (chapter1.md 福利社阿姨 (d), a non-trivial generous choice — same
// GDD §2.2 model as Flag_HelpedSenior's "請學長喝熱咖啡"). Set via the
// Ch1 DialogChoice path; consumed in Ch4 by ResolveOpenerSubState (阿姨
// (a) direct-info vs (d) indirect-info routing) and TryApplyCh4Ripple
// (the +3 情分 callback). Named by the GDD's own LLM input-schema
// example (遊戲企劃與敘事架構.md "Flag_BoughtCoffeeForAuntie_Ch1").
inline constexpr const char* kFlagBoughtCoffeeForAuntie =
    "Flag_BoughtCoffeeForAuntie_Ch1";

// Item 1b — Ch4 finale `!` target hint. The Ch4 roster is gate-driven so
// every NPC ships isQuestGiver=false (ChapterSpawns.h kChapter4), which
// meant the finale chapter painted NO `!` at all and the player had no
// visual cue WHERE the ending is decided. This predicate is the Ch4
// counterpart of Ch3IndicatorVisible: it returns true for 助教 (the NPC
// whose (d) 結算 choice gates Ending A/B/C) until the choice is locked in
// (Flag_TaFinaleChoiceMade), and false for every other npcId — so the
// finale `!` marks exactly the one NPC that advances the spine, and clears
// once the decision is made. View consults it (via QuestIndicatorVisible)
// when state == Chapter4_Finals; because it is keyed on npcId (not on the
// roster's isQuestGiver bit) the gate-driven roster stays byte-unchanged.
[[nodiscard]] bool Ch4IndicatorVisible(std::string_view npcId,
                                       const Player& player);

// E-interact hook, sibling of TryApplyCh2Ripple / TryApplyCh3Ripple.
// Per-NPC, once each (independent keys):
//   西裝學長  HelpedSenior && karma>70 → +10  (chapter4.md L109 (b))
//   學霸      BookwormRecovered        → +5   (L168 (b))
//   助教      HelpedTA_Ch1             → +10  (L228 (b) 坦白)
//   助教      HasProfessorTrap         → -15  (L242 (c) 對峙) — lands
//             INDEPENDENTLY even when (b) is shown (L235「(b) 優先，
//             但 (c) 的 karma 扣點仍計算」), separate key.
//   福利社阿姨 BoughtCoffeeForAuntie_Ch1 → +3  (chapter4.md 阿姨 (a)
//             直接情報 callback：Ch1 情分兌現), once. B3 fix — the
//             Ch1→Ch4 阿姨 ripple the GDD names but engine never read.
// The 助教 (d) 體諒/質問 ±15/-5 is the S5e-2d choice, not here. No-op
// outside Ch4 / wrong npc / unmet flag.
void TryApplyCh4Ripple(Player& player, std::string_view npcId,
                       SemesterState state);

} // namespace nccu

#endif // CHAPTER4_QUEST_H_
