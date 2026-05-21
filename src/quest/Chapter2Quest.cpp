#include "quest/Chapter2Quest.h"
#include "dialog/DialogState.h"
#include "controller/EventBus.h"
#include "entities/Player.h"
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

void TryApplyCh2Ripple(Player& player, std::string_view npcId,
                       SemesterState state) {
    if (state != SemesterState::Chapter2_Midterms) return;

    if (npcId == "suit_senior") {
        if (player.HasFlag(kFlagCh2RippledSuitSenior)) return;  // once
        // HelpedSenior / ScoldedSenior are mutually exclusive (the Ch1
        // C.3(b) choice guard). chapter2.md 學長 (b) `// karma +3`
        // (callback note carries Flag_HelpedSenior=true, already held,
        // so the opener's once-apply skips it) / (c) `// karma -3`
        // (no flag note -> opener never applies it).
        if (player.HasFlag("Flag_HelpedSenior")) {
            player.AddKarma(3).SetFlag(kFlagCh2RippledSuitSenior);
        } else if (player.HasFlag("Flag_ScoldedSenior")) {
            player.AddKarma(-3).SetFlag(kFlagCh2RippledSuitSenior);
        }
        return;
    }

    if (npcId == "ta") {
        if (player.HasFlag(kFlagCh2RippledTA)) return;          // once
        // chapter2.md 助教 (c) `// karma -10`（Ch1 漣漪效應兌現，
        // "karma -10 在此落地"）— a karma-only entry, never auto-
        // applied. (b) HelpedTA_Ch1 is an information ripple with no
        // `// karma`, so there is nothing to land for it (no key set —
        // it never had a karma debt to settle).
        if (player.HasFlag("Flag_HasProfessorTrap")) {
            player.AddKarma(-10).SetFlag(kFlagCh2RippledTA);
        }
        return;
    }
}

} // namespace nccu
