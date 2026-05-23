#ifndef CHAPTER3_QUEST_H_
#define CHAPTER3_QUEST_H_
#include "state/SemesterState.h"
#include <string_view>

class Player;                       // mutated by the trade hooks

namespace nccu {

// Ch3 校慶運動會 物物交換鏈 flags — single source of truth, shared by
// the trade hooks and DialogOpener's Ch3 subState routing so the names
// never drift. Single-use quest items are FLAGS, not the count-only
// consumable inventory (that model is for buyables / the Tab UI;
// 香腸/大聲公 mirror Flag_FoundNote / Flag_FoundForm precedent).
inline constexpr const char* kFlagHasSausage    = "Flag_HasSausage";
inline constexpr const char* kFlagHasLoudspeaker = "Flag_HasLoudspeaker";
inline constexpr const char* kFlagKnowsUmbrellaLoc =
    "Flag_KnowsUmbrellaLoc";

// S5d-3 once-key: the Ch3 ProfessorTrap ripple already landed.
// chapter3.md 章節結尾分支二 L329 「助教旗標若已在 Ch2 扣過，本次 -10
// 獨立計算，不重複」— a SEPARATE key from Flag_Ch2Rippled_TA, so the
// Ch3 -10 lands once even if Ch2 already applied its own.
inline constexpr const char* kFlagCh3RippledProfTrap =
    "Flag_Ch3Rippled_ProfTrap";

// E-interact hook, sibling of TryRescueBookworm / TryApplyCh2Ripple:
// advances the chapter3.md 物物交換鏈 one link per talk, in order.
//   A 系烤香腸攤主  -> Flag_HasSausage,        karma +3 (chain start)
//   B 系大聲公持有者 -> -Sausage +Loudspeaker, karma +3 (link 2)
//   C 系學姊        -> -Loudspeaker +KnowsLoc, karma +5 (info reveal)
// chapter3.md (b)「交易完成」blockquotes carry `// karma` with NO flag
// note, so the opener's once-apply never grants them — path-b, applied
// here. Each link's guard makes it fire exactly once (the chain flag it
// sets gates it out on a re-talk). No-op for any other NPC / state /
// chain position. The TrueUmbrella from the 道具箱 is the actual clear
// (Ch1-isomorphic: claim -> UmbrellaClaimed -> EventWiring Ch3
// sibling-if -> Interlude returnTo Ch4); the chain is the karma /
// narrative path, not a hard gate (mirrors Ch1's optional quest).
void TryAdvanceCh3Trade(Player& player, std::string_view npcId,
                        SemesterState state);

// Sequential quest-giver `!` gate for the A→B→C chain — true only for the
// NEXT link to talk to (A until traded, then B, then C), so the three
// indicators reveal in turn instead of all at once. Returns true for any
// non-chain NPC. View calls this when state == Chapter3_SportsDay.
[[nodiscard]] bool Ch3IndicatorVisible(std::string_view npcId,
                                       const Player& player);

// E-interact hook, sibling of TryApplyCh2Ripple: lands the Ch3
// ProfessorTrap ripple (chapter3.md 章節結尾分支二, `// karma -10`,
// Flag_HasProfessorTrap callback「Ch1 漣漪延伸至 Ch3」). chapter3.md
// uses `- \`// karma\`` bullet docs (not `>` blockquotes), so NOTHING
// in Ch3 is parser-applied — every Ch3 karma is path-b. The narrative
// trigger is 「持 ProfessorTrap 進入體育館後台，後台某個同學認出」;
// there is no backstage NPC section, so this generalises to "the first
// Ch3 NPC notices", applied once per Ch3 via kFlagCh3RippledProfTrap
// (independent of Ch2's key — the -10 is explicitly non-duplicate but
// SEPARATE per chapter, L329). No-op outside Ch3 / without the flag.
void TryApplyCh3Ripple(Player& player, SemesterState state);

} // namespace nccu

#endif // CHAPTER3_QUEST_H_
