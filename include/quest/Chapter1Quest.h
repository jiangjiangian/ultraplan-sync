#ifndef CHAPTER1_QUEST_H_
#define CHAPTER1_QUEST_H_
#include "state/SemesterState.h"
#include <string_view>

class Player;                       // mutated by the return / grant

namespace nccu {

// Ch1 加退選之亂 「善有善報」 quest flags — single source of truth, shared
// by the victim's-umbrella pickup table (ChapterQuestItems), the return
// hook below and DialogOpener's Ch1 victim routing so the names never
// drift (the kFlag* convention of Chapter2Quest.h / Chapter3Quest.h).
//
// Flag_PromisedVictim is set by the (b) 承諾 DialogChoice (chapter1.md 苦主
// (b), `Flag_PromisedVictim = true` / `// karma +5`, applied by
// ApplyDialogChoice), NOT here — it is the existing 承諾 beat and also
// drives the Ch1→Ch3→Ch4 ripple, so it lives with the dialogue. This
// header owns only the NEW reciprocity flag.
//
// Set by the QuestFlagPickup the player finds out in the world (the
// transparent umbrella the 西裝學長 dropped near 集英樓); read by
// TryReturnVictimUmbrella's GRANT phase. Mirrors Flag_HasSausage /
// Flag_FoundForm: a single-use quest item modelled as a flag, not the
// count-only consumable inventory.
inline constexpr const char* kFlagHasVictimUmbrella = "Flag_HasVictimUmbrella";

// E-interact hook: returning the 苦主 (victim) his umbrella in Ch1. The
// reciprocity heart of the redesigned chapter — the chapter clears only
// AFTER the player carries the victim's umbrella BACK to him, never the
// instant an umbrella is grabbed off the ground. Sibling of
// TryRescueBookworm (Ch2) / TryAdvanceCh3Trade (Ch3); invoked from
// GameController's E-interact next to those hooks. No-op unless
// state==Chapter1_AddDrop && npcId=="victim".
//
//   DONE        — already holds Flag_HasTrueUmbrella: no-op (the grant
//                 already happened; a re-talk routes to the (d) recap).
//   NO PROMISE  — !Flag_PromisedVictim: no-op (the (a) plea / (b) 承諾
//                 DialogChoice handles the promise; nothing to return yet).
//   PROMISED,   — has the promise but NOT the victim's umbrella: a
//   NO UMBRELLA   ShowMessage reminder ("先幫他找回那把傘"). No state
//                 change — the player must still find the pickup.
//   GRANT       — has the promise AND Flag_HasVictimUmbrella: the payoff.
//                 Clears Flag_HasVictimUmbrella (the victim takes his傘
//                 back), then REPLICATES TrueUmbrella::beClaimed's effect
//                 — SetHasUmbrella(true) + SetFlag(Flag_HasTrueUmbrella)
//                 (Ending A's precise condition, EndingGate.cpp) — and
//                 publishes, in beClaimed's exact order, the 苦主's
//                 "這是我的傘…我也撿到你的，還你" ShowMessage THEN
//                 UmbrellaClaimed("TrueUmbrella"). The UmbrellaClaimed
//                 subscriber (EventWiring) then transitions Ch1→Interlude
//                 (returnTo Ch2), exactly as the world TrueUmbrella used
//                 to. Idempotent: the grant clears nothing it re-reads, and
//                 the DONE guard above stops a second grant on a re-talk.
//
// The world TrueUmbrella is REMOVED in the redesign (the victim grants it
// now); the Cursed / Fragile / ProfessorTrap umbrellas stay on the ground
// as the morality / Ending-B paths and clear Ch1 via their own beClaimed —
// this hook is the ONLY non-cursed clear path.
void TryReturnVictimUmbrella(Player& player, std::string_view npcId,
                             SemesterState state);

} // namespace nccu

#endif // CHAPTER1_QUEST_H_
