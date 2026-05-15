#ifndef DIALOG_OPENER_H_
#define DIALOG_OPENER_H_
#include "SemesterState.h"
#include <string_view>

namespace nccu {

class DialogState;

// Assembles an NPC's opener from the generated DialogData table and
// pushes it into `dlg`. The (npcId, state, subState) entry's lines
// become the dialog lines. 1b-1: line-only (no choices yet). No
// matching entry -> dlg is left inactive (DialogState::Open({}) is a
// documented no-op).
void OpenNpcDialog(DialogState& dlg, std::string_view npcId,
                   SemesterState state, int subState = 0);

} // namespace nccu
#endif // DIALOG_OPENER_H_
