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

void TryMeetLibrarian(Player& player, std::string_view npcId,
                      SemesterState state) {
    // A2: meeting the 圖書館管理員 is the Ch2 chain head. Her (a) clue line
    // points the player to the 羅馬廣場 statue where the 學霸 is slumped, so
    // talking to her UNLOCKS the 學霸 (the wake step + his dialog gate on this
    // flag). Set at the E-interact moment her line opens — the existing
    // hook idiom (Flag_Bookworm / Flag_SuitSeniorChoiceMade are likewise set
    // at the interaction, not on a dialog-close callback). Idempotent.
    if (state != SemesterState::Chapter2_Midterms) return;
    if (npcId != "librarian") return;
    player.SetFlag(kFlagMetLibrarian);
}

void TryRescueBookworm(Player& player, std::string_view npcId,
                       SemesterState state) {
    if (state != SemesterState::Chapter2_Midterms) return;
    if (npcId != "bookworm") return;
    if (player.HasFlag(kFlagBookwormRecovered)) return;   // already done

    // A2: hard-gate — the player must visit the 圖書館管理員 first (she points
    // to 羅馬廣場). Before that, the 學霸 cannot be woken; the opener routes
    // him to a "先去問櫃台的管理員" redirect, so this nudge backs it up for the
    // E-interact path. No EnergyDrink is consumed (the wake step is below).
    if (!player.HasFlag(kFlagMetLibrarian)) {
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            std::string("這個人睡得很沉……先去問櫃台的管理員吧，"
                        "看看是誰拿走了傘。")});
        return;
    }

    if (!player.HasFlag(kFlagBookworm)) {
        // PHASE 1 — 學霸 still slumped at the 羅馬廣場 statue. The
        // EnergyDrink is consumed HERE, at the wake step (chapter2.md 學霸
        // (c)「玩家有提神飲料時」). Waking sets Flag_Bookworm and the 學霸
        // asks the player to recover his scattered notes — World's
        // deferred spawn then drops the 3 notes, gated on this flag, so NO
        // note exists anywhere before this moment.
        if (player.ConsumeOne("EnergyDrink")) {
            player.SetFlag(kFlagBookworm);
            EventBus::Instance().Publish(Event{
                EventType::ShowMessage,
                std::string("學霸回神了：「筆記……被風吹散了，"
                            "幫我撿回來？」")});
        } else {
            // chapter2.md 學霸 (c-fail) / §五.3 anti-softlock: a player who
            // did not buy an EnergyDrink in the market can still wake him
            // via the 圖書館地下室自動販賣機 (ChapterVendors(Chapter2)).
            EventBus::Instance().Publish(Event{
                EventType::ShowMessage,
                std::string("學霸沒有反應……需要提神飲料喚醒他。"
                            "（圖書館地下室自動販賣機 35 元）")});
        }
        return;
    }

    // PHASE 2 — 學霸 is awake. Gate the exchange on the 3 notes.
    if (!Chapter2NotesComplete(player)) {
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            std::string("「我的筆記還沒撿齊吧？三頁，散在校園各處。」")});
        return;
    }

    // chapter2.md 學霸 (d) `// karma +5` + Ch2 結算旗標
    // Flag_BookwormRecovered. The exchange: notes <-> the player's
    // umbrella. Path-b: the (d) blockquote carries no `Flag_… = true`
    // note, so the opener's once-apply never fires it — it is set here, at
    // the quest-completion code site (the QuestFlagPickup / 換回傘
    // precedent: code sets the milestone flag, the (d) dialog is just the
    // thank-you recap). No EnergyDrink is consumed here — it was spent at
    // the wake step above.
    player.AddKarma(5).SetFlag(kFlagBookwormRecovered);
    // A3 (bag-leak fix): the 3 notes are now HANDED BACK to the 學霸 as part of
    // the exchange — so they must LEAVE the bag. BuildInventoryRows derives the
    // 任務紙張 (學霸的筆記 xN) row purely from Flag_FoundNote1/2/3
    // (ItemCatalog.cpp); without clearing them the owner still saw the 任務紙張
    // row in the market bag AFTER returning the notes. Clear all three here, at
    // the completion site, so the row disappears the next frame the bag is
    // built. Mirrors the carried-item hygiene of the Ch3 trade (Chapter3Quest
    // clears Flag_HasSausage / Flag_HasLoudspeaker the instant each is traded
    // away) and the victim umbrella (TryReturnVictimUmbrella clears
    // Flag_HasVictimUmbrella on the grant). Idempotent — ClearFlag is a no-op
    // if absent, and the kFlagBookwormRecovered early-return above stops a
    // re-talk from re-running this.
    player.ClearFlag(kFlagFoundNote1)
          .ClearFlag(kFlagFoundNote2)
          .ClearFlag(kFlagFoundNote3);
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        std::string("傘換回來了。這次，更換是你。")});
}

void LiftChapter2Clear(Player& player, SemesterState state,
                       const DialogState& dialog) {
    if (state != SemesterState::Chapter2_Midterms) return;
    if (dialog.Active()) return;                       // (d) still on screen
    if (!player.HasFlag(kFlagBookwormRecovered)) return;
    if (player.HasFlag(kFlagCh2Cleared)) return;
    player.SetFlag(kFlagCh2Cleared);
}

void TryLendLibrarianUmbrella(Player& player, std::string_view npcId,
                              SemesterState state) {
    if (state != SemesterState::Chapter2_Midterms) return;
    if (npcId != "librarian") return;
    if (!player.HasFlag(kFlagBookworm)) return;   // her (b) state only
    if (player.HasFlag(kFlagLibrarianUmbrella)) return;  // once — no stack

    // chapter2.md 管理員 (b)「（遞過一把折疊傘）這個先拿著，別在外面淋著。」:
    // the player now HOLDS 管理員的傘. SetHeldUmbrella records the held kind
    // AND sets HasUmbrella(true) so the outdoors-with-umbrella rain path
    // (ApplyRainSheltered) keeps the soak slow while the player hunts the 3
    // notes. Deliberately NOT Flag_HasTrueUmbrella — the loaner must never by
    // itself unlock Ending A. The spoken hand-over lives in the (b) lines
    // (DialogOpener routes 管理員 → (b) on Flag_Bookworm), so no inline
    // ShowMessage is needed here (it would echo the (b) scene).
    player.SetHeldUmbrella(HeldUmbrella::Loaner);
    player.SetFlag(kFlagLibrarianUmbrella);
}

void TryReturnLibrarianUmbrella(Player& player, std::string_view npcId,
                                SemesterState state, SemesterState returnTo) {
    // G-3: scope HARD to the Ch2→Ch3 Interlude return-point. The market that
    // returns to Ch3 is the only one where the player can still be holding the
    // 管理員 loaner (it was lent in Ch2 and is cleared on the NEXT chapter
    // entry — SceneRouter), so InterludeReturnTo()==Chapter3_SportsDay is the
    // exact gate. Any other state / market / npcId is a no-op.
    if (state != SemesterState::Interlude_Market) return;
    if (returnTo != SemesterState::Chapter3_SportsDay) return;
    if (npcId != kNpcLibrarianReturn) return;

    if (player.HasFlag(kFlagLibrarianUmbrellaReturned)) {
        // Re-talk after returning: a short closure line, NO second karma.
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            std::string("這把傘已經還給圖書館了。")});
        return;
    }

    // Defensive: the marker only spawns while the player holds the loaner, but
    // re-check both the flag AND the held kind so a stray talk can't grant the
    // +10 without actually surrendering 管理員的傘.
    if (!player.HasFlag(kFlagLibrarianUmbrella) ||
        player.HeldUmbrellaKind() != HeldUmbrella::Loaner) {
        return;
    }

    // The 責任感 payoff: hand the loaner back. karma +10, the held umbrella is
    // emptied (SetHasUmbrella(false) also clears heldUmbrella_), the lent latch
    // is cleared, and the once-key is set so this fires exactly once. NO ending
    // flag is touched — the loaner was never Flag_HasTrueUmbrella, so Ending A
    // stays unaffected.
    player.AddKarma(10)
          .SetHasUmbrella(false)
          .ClearFlag(kFlagLibrarianUmbrella)
          .SetFlag(kFlagLibrarianUmbrellaReturned);
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        std::string("你把管理員的傘還回了圖書館服務台。"
                    "「傘有借有還，謝謝你特地拿回來。」")});
}

bool Ch2IndicatorVisible(std::string_view npcId, bool isQuestGiver,
                         const Player& player) {
    const bool woken     = player.HasFlag(kFlagBookworm);
    const bool recovered = player.HasFlag(kFlagBookwormRecovered);
    if (npcId == "librarian")
        return !woken;                 // chain head: lit until 學霸 is woken
    if (npcId == "bookworm")
        return woken && !recovered;    // lit after waking, through 換回
    // Any other NPC: unchanged — honour its roster isQuestGiver bit.
    return isQuestGiver;
}

void TryApplyCh2Ripple(Player& player, std::string_view npcId,
                       SemesterState state) {
    if (state != SemesterState::Chapter2_Midterms) return;

    if (npcId == "suit_senior") {
        if (player.HasFlag(kFlagCh2RippledSuitSenior)) return;  // once
        // HelpedSenior / ScoldedSenior are mutually exclusive (the Ch1
        // C.3(b) choice guard). chapter2.md 學長 (b) `// karma +3`
        // (callback note carries Flag_HelpedSenior=true, already held,
        // so the opener's once-apply skips it).
        //
        // T1 reframe (CHANGELOG): the Ch1 (b) choice is no longer a hostile
        // 斥責 (-5) but a RATIONAL call-out (+3). Flag_ScoldedSenior is kept
        // only as the "保持距離" arc KEY (Ch2 (c) 尷尬讓開 / Ch3 距離 / Ch4
        // 不主動出場); the senior keeps a respectful distance out of mild
        // embarrassment, NOT resentment, so it carries NO karma penalty —
        // a follow-on -3 would wrongly claw back the rational +3 the choice
        // just earned. The flag is still consumed via the once-key so the
        // arc routes exactly once; only the karma debt is gone.
        if (player.HasFlag(kFlagHelpedSenior)) {
            player.AddKarma(3).SetFlag(kFlagCh2RippledSuitSenior);
        } else if (player.HasFlag(kFlagScoldedSenior)) {
            player.SetFlag(kFlagCh2RippledSuitSenior);          // karma-neutral
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
        if (player.HasFlag(kFlagHasProfessorTrap)) {
            player.AddKarma(-10).SetFlag(kFlagCh2RippledTA);
        }
        return;
    }
}

} // namespace nccu
