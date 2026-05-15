#include "EndingGate.h"
#include "Player.h"
#include "SemesterStateMachine.h"
#include "SemesterState.h"

namespace nccu {

void CheckEndingGates(Player& player, SemesterStateMachine& semester) {
    if (semester.Current() == SemesterState::Chapter1_AddDrop &&
        player.HasFlag("Flag_BoughtUglyUmbrella")) {
        semester.Transition(SemesterState::Ending_C);
    }
}

} // namespace nccu
