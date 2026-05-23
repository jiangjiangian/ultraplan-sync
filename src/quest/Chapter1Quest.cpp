#include "quest/Chapter1Quest.h"
#include "controller/EventBus.h"
#include "entities/Player.h"
#include <string>

namespace nccu {

void TryReturnVictimUmbrella(Player& player, std::string_view npcId,
                             SemesterState state) {
    if (state != SemesterState::Chapter1_AddDrop) return;
    if (npcId != "victim") return;
    if (player.HasFlag("Flag_HasTrueUmbrella")) return;   // already granted

    if (!player.HasFlag("Flag_PromisedVictim")) {
        // The (a) plea / (b) 承諾 DialogChoice owns the promise beat; there
        // is nothing to return before the player has even agreed to help.
        // No-op so the opener routes to (a)/(b) untouched.
        return;
    }

    if (!player.HasFlag(kFlagHasVictimUmbrella)) {
        // Promised but empty-handed — nudge toward the findable umbrella the
        // 西裝學長 dropped near 集英樓 (the QuestFlagPickup). No state change.
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            std::string("「找到我的傘了嗎？先幫他找回那把傘吧——"
                        "聽說那個西裝學長往集英樓跑了。」")});
        return;
    }

    // GRANT — the reciprocity payoff. The player carried the victim's
    // umbrella back; the 苦主 takes it, then reveals he ALSO found the
    // player's true umbrella and hands it over. This REPLICATES
    // TrueUmbrella::beClaimed's effect (the world TrueUmbrella is removed in
    // the redesign — the victim is the only non-cursed grant): set
    // HasUmbrella + Flag_HasTrueUmbrella (Ending A's precise condition,
    // EndingGate.cpp), then publish in beClaimed's EXACT order — ShowMessage
    // FIRST, UmbrellaClaimed SECOND — so the EventWiring UmbrellaClaimed
    // subscriber's chapter-clear toast is the banner that wins the single
    // HUD slot (reversing the pair re-introduces the Cycle 9.A.2 regression
    // pinned by tests/quest/test_chapter_transitions.cpp). The
    // UmbrellaClaimed("TrueUmbrella") then drives Ch1→Interlude (returnTo
    // Ch2) via the EventWiring Ch1 sibling-if.
    player.ClearFlag(kFlagHasVictimUmbrella);   // 苦主 takes his傘 back
    player.SetHasUmbrella(true);
    player.SetFlag("Flag_HasTrueUmbrella");
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        std::string("苦主：「這就是我的傘，謝謝你！對了——"
                    "我剛剛也撿到你那把，還你。」")});
    EventBus::Instance().Publish(Event{
        EventType::UmbrellaClaimed, std::string("TrueUmbrella")});
}

} // namespace nccu
