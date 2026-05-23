#include "quest/Chapter4Quest.h"
#include "entities/Player.h"

namespace nccu {

// Item 1b: 助教 is the finale `!` until the (d) 結算 choice is committed
// (Flag_TaFinaleChoiceMade, set by GameController on confirm; the 1c exit
// does NOT set it, so backing out keeps the `!` lit for a re-approach).
// Every other Ch4 npcId is dark — the finale is gate-driven, so a `!` on
// the ripple-flavour archetypes would be pure noise.
bool Ch4IndicatorVisible(std::string_view npcId, const Player& player) {
    if (npcId == "ta") return !player.HasFlag("Flag_TaFinaleChoiceMade");
    return false;
}

void TryGrantTaFinaleUmbrella(Player& player, std::string_view npcId,
                              SemesterState state) {
    if (state != SemesterState::Chapter4_Finals) return;
    if (npcId != "ta") return;
    if (!player.HasFlag("Flag_ConsoledTA")) return;   // gentle branch only
    if (player.HasFlag("Flag_HasTrueUmbrella")) return;  // idempotent
    // The 助教 hands YOUR umbrella back when you're kind — Ending A's 持傘
    // condition on the gentle path. EndingGate keeps the karma>80 gate, so
    // this does not by itself force Ending A; it only makes 體諒+high-karma
    // sufficient WITHOUT the hidden Ch4 umbrella.
    player.SetHasUmbrella(true);
    player.SetFlag("Flag_HasTrueUmbrella");
}

void TryApplyCh4Ripple(Player& player, std::string_view npcId,
                       SemesterState state) {
    if (state != SemesterState::Chapter4_Finals) return;

    if (npcId == "suit_senior") {
        // (b) karma>70 崩潰坦白 `// karma +10` — only on the (b) route
        // (HelpedSenior && karma>70), once.
        if (player.HasFlag(kFlagCh4RippledSenior)) return;
        if (player.HasFlag("Flag_HelpedSenior") && player.GetKarma() > 70)
            player.AddKarma(10).SetFlag(kFlagCh4RippledSenior);
        return;
    }

    if (npcId == "bookworm") {
        // (b) Ch2 救他 callback `// karma +5`, once.
        if (player.HasFlag(kFlagCh4RippledBookworm)) return;
        if (player.HasFlag("Flag_BookwormRecovered"))
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
            player.HasFlag("Flag_HelpedTA_Ch1"))
            player.AddKarma(10).SetFlag(kFlagCh4RippledTAHelped);
        if (!player.HasFlag(kFlagCh4RippledProfTrap) &&
            player.HasFlag("Flag_HasProfessorTrap"))
            player.AddKarma(-15).SetFlag(kFlagCh4RippledProfTrap);
        return;
    }
}

} // namespace nccu
