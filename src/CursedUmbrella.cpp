#include "CursedUmbrella.h"
#include "Player.h"
#include "EventBus.h"

void CursedUmbrella::beClaimed(Player* player) {
    if (player == nullptr) return;
    if (!isActive_) return;        // idempotent: a second call is a no-op
    // Fluent mutators — all return Player&, so the state changes read as
    // one atomic transaction. SetFlag is the ripple seed (F.9-b): taking
    // the cursed umbrella in Ch1 is what drives Ending B and the Ch2-4
    // 學霸/環境 cold reactions; without it that path was unreachable.
    player->SetHasUmbrella(true)
           .decreaseKarma(karmaPenalty_)
           .SetFlag("Flag_TookCursedUmbrella");
    isActive_ = false;
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "CursedUmbrella" });
    EventBus::Instance().Publish(Event{ EventType::KarmaChanged, "Karma -30" });
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "你順手牽羊了！成為了你最討厭的人。" });
}
