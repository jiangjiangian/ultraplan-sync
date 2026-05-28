#include "controller/DialogChoiceApply.h"
#include "dialog/DialogState.h"   // DialogChoice
#include "entities/Player.h"

namespace nccu {

void ApplyDialogChoice(Player& player, const DialogChoice& choice) {
    // Item 5a — see header comment. Skip both the karma and the flag
    // write if the choice would re-grant a one-off reward.
    if (!choice.setsFlag.empty() && choice.flagValue &&
        player.HasFlag(choice.setsFlag)) {
        return;   // reward already collected — no double-dip
    }
    player.AddKarma(choice.karmaDelta);
    if (!choice.setsFlag.empty()) {
        if (choice.flagValue) player.SetFlag(choice.setsFlag);
        else                  player.ClearFlag(choice.setsFlag);
    }
}

} // namespace nccu
