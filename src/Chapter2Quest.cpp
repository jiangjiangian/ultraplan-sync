#include "Chapter2Quest.h"
#include "DialogState.h"
#include "EventBus.h"
#include "Player.h"
#include <string>

namespace nccu {

bool Chapter2NotesComplete(const Player& player) {
    return player.HasFlag(kFlagFoundNote1) &&
           player.HasFlag(kFlagFoundNote2) &&
           player.HasFlag(kFlagFoundNote3);
}

void TryRescueBookworm(Player& player, std::string_view npcId,
                       SemesterState state) {
    if (state != SemesterState::Chapter2_Midterms) return;
    if (npcId != "bookworm") return;
    if (player.HasFlag(kFlagBookwormRecovered)) return;   // already done
    if (!Chapter2NotesComplete(player)) return;            // notes first

    if (player.ConsumeOne("EnergyDrink")) {
        // chapter2.md 學霸 (d) `// karma +5` + Ch2 結算旗標
        // Flag_BookwormRecovered. Path-b: the (d) blockquote carries no
        // `Flag_… = true` note, so the opener's once-apply never fires
        // it — it is set here, at the quest-completion code site (the
        // QuestFlagPickup / 換回傘 precedent: code sets the milestone
        // flag, the (d) dialog is just the thank-you recap).
        player.AddKarma(5).SetFlag(kFlagBookwormRecovered);
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            std::string("傘換回來了。這次，更換是你。")});
    } else {
        // chapter2.md 學霸 (c-fail) / §五.3 anti-softlock: a player who
        // did not buy an EnergyDrink in the market can still finish Ch2
        // via the 圖書館地下室自動販賣機 (ChapterVendors(Chapter2)).
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            std::string("學霸沒有反應……需要提神飲料喚醒他。"
                        "（圖書館地下室自動販賣機 35 元）")});
    }
}

void LiftChapter2Clear(Player& player, SemesterState state,
                       const DialogState& dialog) {
    if (state != SemesterState::Chapter2_Midterms) return;
    if (dialog.Active()) return;                       // (d) still on screen
    if (!player.HasFlag(kFlagBookwormRecovered)) return;
    if (player.HasFlag(kFlagCh2Cleared)) return;
    player.SetFlag(kFlagCh2Cleared);
}

} // namespace nccu
