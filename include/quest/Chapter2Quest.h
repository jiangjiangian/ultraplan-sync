#ifndef CHAPTER2_QUEST_H_
#define CHAPTER2_QUEST_H_
#include "state/SemesterState.h"
#include <string_view>

class Player;                       // mutated by the rescue / lift

namespace nccu {

class DialogState;                  // queried (Active) by the lift

// Ch2 圖書館期中考 quest flags — the single source of truth, shared by
// the note-pickup table (ChapterQuestItems), DialogOpener's subState
// routing and the rescue/lift below so the names never drift.
inline constexpr const char* kFlagFoundNote1        = "Flag_FoundNote1";
inline constexpr const char* kFlagFoundNote2        = "Flag_FoundNote2";
inline constexpr const char* kFlagFoundNote3        = "Flag_FoundNote3";
inline constexpr const char* kFlagBookwormRecovered = "Flag_BookwormRecovered";
inline constexpr const char* kFlagCh2Cleared        = "Flag_Ch2Cleared";

// S5c-3 per-NPC "the Ch2 ripple karma already landed" once-keys. The
// ripple dialog itself may re-show on re-talks (a consistent reaction);
// only the karma adjustment is gated to fire exactly once per Ch2.
inline constexpr const char* kFlagCh2RippledSuitSenior =
    "Flag_Ch2Rippled_SuitSenior";
inline constexpr const char* kFlagCh2RippledTA = "Flag_Ch2Rippled_TA";

// All three 學霸 notes collected — gates 圖書館管理員 (b) and the rescue.
[[nodiscard]] bool Chapter2NotesComplete(const Player& player);

// E-interact hook: talking to 學霸 (bookworm) at the rescue moment.
// No-op unless state==Chapter2_Midterms, npcId=="bookworm", the 3 notes
// are in, and 學霸 is not yet recovered. With an EnergyDrink in the
// count-inventory it is consumed -> Flag_BookwormRecovered + karma +5
// (chapter2.md 學霸 (d) `// karma +5`; path-b — applied here because the
// (d) blockquote carries no flag annotation, so the opener's once-apply
// would never fire it). Without one, a hint to the 圖書館地下室自販機
// fallback is shown and nothing is consumed. Deliberately does NOT set
// Flag_Ch2Cleared — that is lifted later so the (d) thanks dialog is
// readable first (see LiftChapter2Clear).
void TryRescueBookworm(Player& player, std::string_view npcId,
                       SemesterState state);

// Deferred chapter-clear lift. Sets Flag_Ch2Cleared only once 學霸 is
// recovered AND no dialog is open — i.e. after the player has finished
// reading the (d) thanks. CheckChapterGates' existing Ch2 sibling-if
// then transitions to the Interlude. Decoupling the clear from the
// rescue interact stops the gate from closing the (d) dialog on the
// same frame it opens (mirrors Flag_LeaveInterlude: trigger, then a
// polled consume).
void LiftChapter2Clear(Player& player, SemesterState state,
                       const DialogState& dialog);

// E-interact hook, sibling of TryRescueBookworm — lands the Ch1->Ch2
// ripple karma the dialog opener cannot. chapter2.md routes 西裝學長 /
// 助教 to a flag-gated subState whose `// karma` the opener's once-apply
// will NOT grant (it is guarded behind a NOT-yet-set flag, but the
// ripple flags are Ch1 flags already held — or the entry is karma-only
// with no flag). So the documented "karma -10 在此落地" / ±3 must be
// applied here, once per Ch2 (per-NPC once-key). No-op for any other
// NPC / state / flag combination. Precedence (chapter2.md L211/L225):
//   助教   HasProfessorTrap (c, 取代 a/b, -10)  >  HelpedTA_Ch1 (b, 0)
//   西裝學長 HelpedSenior (b, +3)  xor  ScoldedSenior (c, -3)
void TryApplyCh2Ripple(Player& player, std::string_view npcId,
                       SemesterState state);

} // namespace nccu

#endif // CHAPTER2_QUEST_H_
