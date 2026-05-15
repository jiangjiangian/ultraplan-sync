#ifndef ENDING_GATE_H_
#define ENDING_GATE_H_
class Player;                 // global-namespace model object
namespace nccu {
class SemesterStateMachine;
class DialogState;
// Flag-driven chapter/ending gates, checked right after a dialog choice
// is applied. Ch1: buying the ugly green umbrella (Flag_BoughtUglyUmbrella
// set via 福利社阿姨's choice) -> Ending C. Sibling-if shape, same as the
// 1c UmbrellaClaimed gate; future endings = add sibling ifs here, do not
// generalise. The GDD's extra "coins==0" condition is Phase 2 (no money
// economy yet) — 1d wires the flag condition only. On any transition it
// also Close()s the open conversation so a stale Active dialog can't keep
// eating input behind the terminal screen.
void CheckEndingGates(Player& player, SemesterStateMachine& semester,
                      DialogState& dialog);
}
#endif // ENDING_GATE_H_
