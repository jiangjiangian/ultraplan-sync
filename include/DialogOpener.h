#ifndef DIALOG_OPENER_H_
#define DIALOG_OPENER_H_
#include "SemesterState.h"
#include <string_view>

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

} // namespace nccu
#endif // DIALOG_OPENER_H_
