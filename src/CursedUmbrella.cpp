#include "CursedUmbrella.h"
#include "Player.h"
#include "EventBus.h"

void CursedUmbrella::beClaimed(Player* player) {
    if (player == nullptr) return;
    if (!isActive_) return;        // idempotent: a second call is a no-op
    // Fluent mutators — both return Player&, so the two state changes
    // read as one atomic transaction at the call site.
    player->SetHasUmbrella(true).decreaseKarma(karmaPenalty_);
    isActive_ = false;
    EventBus::Instance().Publish(Event{
        EventType::UmbrellaClaimed,
        position_,
        umbrellaTint_,
        "CursedUmbrella"
    });
    EventBus::Instance().Publish(Event{
        EventType::KarmaChanged,
        position_,
        nccu::gfx::Colors::White,
        "Karma -50"
    });
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        position_,
        nccu::gfx::Colors::White,
        "你順手牽羊了！成為了你最討厭的人。"
    });
}
