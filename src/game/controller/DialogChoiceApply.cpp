#include "game/controller/DialogChoiceApply.h"
#include "game/dialog/DialogState.h"   // DialogChoice
#include "game/entities/Player.h"

namespace nccu {

void ApplyDialogChoice(Player& player, const DialogChoice& choice) {
    // 詳見標頭註解。若此選項會「重新」給予一次性獎勵，則同時略過業力與旗標寫入。
    if (!choice.setsFlag.empty() && choice.flagValue &&
        player.HasFlag(choice.setsFlag)) {
        return;   // 獎勵已領過——不重複領取
    }
    player.AddKarma(choice.karmaDelta);
    if (!choice.setsFlag.empty()) {
        if (choice.flagValue) player.SetFlag(choice.setsFlag);
        else                  player.ClearFlag(choice.setsFlag);
    }
}

} // namespace nccu
