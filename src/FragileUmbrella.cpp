#include "FragileUmbrella.h"
#include "Player.h"
#include "EventBus.h"

void FragileUmbrella::beClaimed(Player* player) {
    if (!player) return;
    player->SetHasUmbrella(true);
    isActive_ = false;
    EventBus::Instance().Publish(Event{
        EventType::UmbrellaClaimed,
        position_,
        umbrellaTint_,
        "FragileUmbrella"
    });
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        position_,
        nccu::gfx::Colors::White,
        "你撿到了 FragileUmbrella，骨架斷了，雨還是會慢慢滲進來。"
    });
}
