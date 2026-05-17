#include "TrueUmbrella.h"
#include "Player.h"
#include "EventBus.h"

void TrueUmbrella::beClaimed(Player* player) {
    if (!player) return;
    player->SetHasUmbrella(true);
    // TrueUmbrella-specific marker (S5e-2b): HasUmbrella() is set by
    // EVERY umbrella, so it cannot tell Ending A's 持-TrueUmbrella
    // apart. This flag is TrueUmbrella-only; GameController clears it
    // on Ch4 entry (chapter4.md 傘再度失蹤), so in Ch4 it means exactly
    // "re-claimed the Ch4 TrueUmbrella" — Ending A's precise condition,
    // with no leak from a stray ctor Fragile/ProfTrap.
    player->SetFlag("Flag_HasTrueUmbrella");
    isActive_ = false; // mark for end-of-frame sweep
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "TrueUmbrella" });
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "你撿到了 TrueUmbrella，雨停了。" });
}
