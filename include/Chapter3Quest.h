#ifndef CHAPTER3_QUEST_H_
#define CHAPTER3_QUEST_H_
#include "SemesterState.h"
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

} // namespace nccu

#endif // CHAPTER3_QUEST_H_
