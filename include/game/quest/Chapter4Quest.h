#ifndef CHAPTER4_QUEST_H_
#define CHAPTER4_QUEST_H_
#include "game/quest/Flags.h"
#include "game/state/SemesterState.h"
#include <string_view>

class Player;

namespace nccu {

class DialogState;   // mutated by TryOpenEndingConfession (opens the 自白)

// Ch4 期末考終焉 peak-ripple usage notes. The `kFlag*` constants live in
// `quest/Flags.h`. chapter4.md karma is `- ` bullet-doc (not `>`
// blockquote) → NOTHING is parser-applied; every Ch4 karma is path-b. The
// kFlagCh4Rippled* once-keys land the reactive ripple karma the (a)/(b)/(c)
// routing (S5e-2a, line-only) cannot, once per Ch4 per effect, so the
// virtuous path can actually clear karma>80 for Ending A and the
// ProfessorTrap arc cashes in its final -15.
//
// kFlagBoughtCoffeeForAuntie — Ch1 漣漪種子: the player bought 福利社阿姨
// a hot coffee in Chapter 1 (chapter1.md 福利社阿姨 (d), a non-trivial
// generous choice — same GDD §2.2 model as Flag_HelpedSenior's "請學長喝
// 熱咖啡"). Set via the Ch1 DialogChoice path; consumed in Ch4 by
// ResolveOpenerSubState (阿姨 (a) direct-info vs (d) indirect-info routing)
// and TryApplyCh4Ripple (the +3 情分 callback). Named by the GDD's own
// LLM input-schema example (遊戲企劃與敘事架構.md "Flag_BoughtCoffeeForAuntie_Ch1").

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

// T4: the gentle-finale umbrella grant. When the player chose 體諒 with
// the 助教 (Flag_ConsoledTA, set by the S5e-2d choice's ApplyDialogChoice),
// the 助教 presses the player's true umbrella back into their hands — so
// the gentle path sets Flag_HasTrueUmbrella + HasUmbrella (the spoken
// "拿回你的傘" beat is in the choice's nextLines). This is Ending A's 持傘
// condition: with karma>80, 體諒 alone now reaches Ending A, parallel to
// (and not requiring) the hidden Ch4 umbrella. The harsh 質問 branch never
// sets Flag_ConsoledTA, so it never reaches here and resolves to Ending B
// (coldFinale). Idempotent (HasFlag guard). No-op outside Ch4 / wrong npc /
// before 體諒. Called by GameController on a confirmed 助教 finale choice,
// AFTER ApplyDialogChoice has landed Flag_ConsoledTA.
void TryGrantTaFinaleUmbrella(Player& player, std::string_view npcId,
                              SemesterState state);

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

// kFlagCh4ConfessedCursed / kFlagCh4ConfessedUgly / kFlagCh4ConfessedTrue
// — G2 once-keys for the Ch4 ending 自白 (inner monologue). Each ending
// trigger plays its brief confession EXACTLY once before the gate resolves
// (the gate is deferred behind any active dialog), so the ending emerges
// coherently instead of snapping on the raw trigger frame. Code-only flags
// (no content reference) — set by TryOpenEndingConfession.

// G2 — defer each Ch4 ending behind a short inner-monologue (自白) so the
// resolution is coherent, not abrupt. Polled by GameController every
// NON-dialog frame, BEFORE CheckEndingGates (which itself early-returns
// while a dialog is active). When an ending-deciding state is reached in
// Ch4 and no confession for it has played yet, this opens a brief 自白
// DialogState and sets that path's once-key; the now-active dialog makes
// CheckEndingGates defer until the player closes it, then the gate fires.
// Covers the three Ch4 ending TRIGGERS that lack their own closing beat:
//   • Flag_TookCursedUmbrella (carried from Ch1) → the curse-caught-up 自白
//     before Ending B. (Cold finale / 質問 already has its own nextLines.)
//   • Flag_BoughtUglyUmbrella → the 務實 self-talk before Ending C.
//   • Flag_HasTrueUmbrella reclaimed from the GROUND (the hidden gym
//     umbrella) WITHOUT yet doing the 助教 finale → the relief 自白. The
//     gentle 體諒 finale grants the same flag but plays its OWN beat (the
//     choice's nextLines), so this is gated on !Flag_TaFinaleChoiceMade to
//     avoid a double narration there.
// Returns true if it opened a confession this call (a dialog is now up).
// Idempotent (once-keys); no-op outside Ch4 / while a dialog is active /
// after the matching confession has played. Precedence mirrors the gate
// (cursed → B over the others) so the 自白 matches the ending that fires.
bool TryOpenEndingConfession(Player& player, DialogState& dialog,
                             SemesterState state);

} // namespace nccu

#endif // CHAPTER4_QUEST_H_
