#include "HotPack.h"
#include "Player.h"
#include "EventBus.h"

void HotPack::Consume(Player* player) {
    if (!player) return;
    player->AddKarma(kKarmaBonus).resetRainMeter();
    isActive_ = false;
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        position_,
        nccu::gfx::Colors::White,
        "用了暖暖包，雨水蒸發了，心情也好了一些。"
    });
}
