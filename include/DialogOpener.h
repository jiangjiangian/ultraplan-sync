#ifndef DIALOG_OPENER_H_
#define DIALOG_OPENER_H_
#include "SemesterState.h"
#include <string_view>

class Player;  // global-namespace model object

namespace nccu {

class DialogState;

// Line-only opener: the (npcId, state, subState) entry's lines become
// the dialog lines, no choices. No matching entry -> dlg is left
// inactive (DialogState::Open({}) is a documented no-op).
void OpenNpcDialogSub(DialogState& dlg, std::string_view npcId,
                      SemesterState state, int subState);

// Smart entry point. Builds the subState-0 opener lines and, for NPCs
// that branch in this state, one DialogChoice per sibling subState >=1
// (label = choiceLabel, karma/flag from that entry, nextLines = its
// lines). Non-branching NPCs stay line-only. No subState-0 entry ->
// dlg left inactive.
void OpenNpcDialog(DialogState& dlg, std::string_view npcId,
                   SemesterState state);

// Picks which subState is the opener for (npcId, state) given quest
// flags. Ch1: ta -> 1 once Flag_FoundForm (reward) and stays 1 after
// Flag_HelpedTA_Ch1; victim -> 1 once Flag_PromisedVictim (recap, no
// re-choice); everyone else -> 0. Pure; no mutation.
int ResolveOpenerSubState(std::string_view npcId, SemesterState state,
                          const Player& player);

// Player-aware entry point (the one GameController calls). Resolves the
// opener subState; subState 0 delegates to the 3-arg OpenNpcDialog
// (preserving the 1b-2 choice path); subState >=1 opens that entry
// line-only AND applies its karma/flag ONCE (guard: only when the
// entry sets a true flag not yet on the player).
void OpenNpcDialog(DialogState& dlg, Player& player,
                   std::string_view npcId, SemesterState state);

} // namespace nccu
#endif // DIALOG_OPENER_H_
