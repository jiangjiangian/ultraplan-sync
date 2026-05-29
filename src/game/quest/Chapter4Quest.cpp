#include "game/quest/Chapter4Quest.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"

namespace nccu {

// Item 1b: 助教 is the finale `!` until the (d) 結算 choice is committed
// (Flag_TaFinaleChoiceMade, set by GameController on confirm; the 1c exit
// does NOT set it, so backing out keeps the `!` lit for a re-approach).
// Every other Ch4 npcId is dark — the finale is gate-driven, so a `!` on
// the ripple-flavour archetypes would be pure noise.
bool Ch4IndicatorVisible(std::string_view npcId, const Player& player) {
    if (npcId == "ta") return !player.HasFlag(kFlagTaFinaleChoiceMade);
    return false;
}

void TryGrantTaFinaleUmbrella(Player& player, std::string_view npcId,
                              SemesterState state) {
    if (state != SemesterState::Chapter4_Finals) return;
    if (npcId != "ta") return;
    if (!player.HasFlag(kFlagConsoledTA)) return;   // gentle branch only
    if (player.HasFlag(kFlagHasTrueUmbrella)) return;  // idempotent
    // The 助教 hands YOUR umbrella back when you're kind — Ending A's 持傘
    // condition on the gentle path. EndingGate keeps the karma>80 gate, so
    // this does not by itself force Ending A; it only makes 體諒+high-karma
    // sufficient WITHOUT the hidden Ch4 umbrella.
    // B2.1: SetHeldUmbrella records the held true umbrella (and sets
    // HasUmbrella) so the bag shows the 真傘 row after the gentle finale.
    player.SetHeldUmbrella(HeldUmbrella::True);
    player.SetFlag(kFlagHasTrueUmbrella);
}

void TryApplyCh4Ripple(Player& player, std::string_view npcId,
                       SemesterState state) {
    if (state != SemesterState::Chapter4_Finals) return;

    if (npcId == "suit_senior") {
        // (b) karma>70 崩潰坦白 `// karma +10` — only on the (b) route
        // (HelpedSenior && karma>70), once.
        if (player.HasFlag(kFlagCh4RippledSenior)) return;
        if (player.HasFlag(kFlagHelpedSenior) && player.GetKarma() > 70)
            player.AddKarma(10).SetFlag(kFlagCh4RippledSenior);
        return;
    }

    if (npcId == "bookworm") {
        // (b) Ch2 救他 callback `// karma +5`, once.
        if (player.HasFlag(kFlagCh4RippledBookworm)) return;
        if (player.HasFlag(kFlagBookwormRecovered))
            player.AddKarma(5).SetFlag(kFlagCh4RippledBookworm);
        return;
    }

    if (npcId == "shop_auntie") {
        // chapter4.md 阿姨 (a) 直接情報 callback `// karma +3`（Ch1
        // 請咖啡情分，Ch4 直接情報兌現）, once. Only on the direct-info
        // route (Flag_BoughtCoffeeForAuntie_Ch1) — the (d) 間接情報
        // route grants nothing, mirroring 學長 +10 being (b)-route only.
        if (player.HasFlag(kFlagCh4RippledAuntie)) return;
        if (player.HasFlag(kFlagBoughtCoffeeForAuntie))
            player.AddKarma(3).SetFlag(kFlagCh4RippledAuntie);
        return;
    }

    if (npcId == "ta") {
        // Two INDEPENDENT effects (chapter4.md L235): the (b) 坦白 +10
        // for HelpedTA_Ch1 AND the (c) 對峙 -15 for HasProfessorTrap —
        // when both hold, (b) shows but the -15 still lands. Separate
        // once-keys so a HelpedTA+ProfTrap player nets +10-15 = -5.
        if (!player.HasFlag(kFlagCh4RippledTAHelped) &&
            player.HasFlag(kFlagHelpedTACh1))
            player.AddKarma(10).SetFlag(kFlagCh4RippledTAHelped);
        if (!player.HasFlag(kFlagCh4RippledProfTrap) &&
            player.HasFlag(kFlagHasProfessorTrap))
            player.AddKarma(-15).SetFlag(kFlagCh4RippledProfTrap);
        return;
    }
}

bool TryOpenEndingConfession(Player& player, DialogState& dialog,
                             SemesterState state) {
    if (state != SemesterState::Chapter4_Finals) return false;
    if (dialog.Active()) return false;   // never interrupt an open box

    // Precedence mirrors CheckEndingGates (cursed → B outranks D/C), so the
    // confession the player reads is the one that will actually fire. Each
    // is one-shot via its own once-key; an already-confessed trigger is
    // skipped so a later poll resolves the (deferred) gate instead.

    // Cursed-caught-up 自白 — the Flag_TookCursedUmbrella carried from Ch1
    // resolves to Ending B in Ch4. Give it a beat before the doom.
    if (player.HasFlag(kFlagTookCursedUmbrella) &&
        !player.HasFlag(kFlagCh4ConfessedCursed)) {
        player.SetFlag(kFlagCh4ConfessedCursed);
        dialog.Open({
            "（你握著那把刻著別人名字的傘，傘骨還在發出細微的嗡鳴）",
            "（從加退選那天起，雨就沒真正停過，它一直跟著你）",
            "（你心裡很清楚——你早就成了你最不想變成的那種人）"});
        return true;
    }

    // 務實 自白 — bought the 螢光綠醜傘; resolves to Ending C.
    if (player.HasFlag(kFlagBoughtUglyUmbrella) &&
        !player.HasFlag(kFlagCh4ConfessedUgly)) {
        player.SetFlag(kFlagCh4ConfessedUgly);
        dialog.Open({
            "（你撐開那把醜得理直氣壯的螢光綠傘）",
            "（沒人會想跟你拿錯，這場雨你是花錢買過的）",
            "（算了，傘能擋雨就好。其他的，就這樣吧。）"});
        return true;
    }

    // Reclaimed-true 自白 — found the hidden 體育館 真傘 from the ground,
    // BEFORE the 助教 finale (the gentle finale plays its own nextLines, so
    // gate on !Flag_TaFinaleChoiceMade to avoid a double beat there).
    if (player.HasFlag(kFlagHasTrueUmbrella) &&
        !player.HasFlag(kFlagTaFinaleChoiceMade) &&
        !player.HasFlag(kFlagCh4ConfessedTrue)) {
        player.SetFlag(kFlagCh4ConfessedTrue);
        dialog.Open({
            "（你的手指扣上那道熟悉的弧度——傘骨紮實，沒有一根是歪的）",
            "（找了一整個學期，繞了那麼遠，它終究回到你手上）",
            "（雨好像小了一點。剩下的路，你想好好走完。）"});
        return true;
    }

    return false;
}

} // namespace nccu
