#include "TrueUmbrella.h"
#include "Player.h"
#include "EventBus.h"

void TrueUmbrella::beClaimed(Player* player) {
    if (!player) return;
    player->SetHasUmbrella(true);
    isActive_ = false; // mark for end-of-frame sweep
    EventBus::Instance().Publish(Event{
        EventType::UmbrellaClaimed,
        position_,
        umbrellaTint_,
        "TrueUmbrella"
    });
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        position_,
        nccu::gfx::Colors::White,
        "你撿到了 TrueUmbrella，雨停了。"
    });
}
