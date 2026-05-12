#include "EnergyDrink.h"
#include "Player.h"
#include "EventBus.h"

void EnergyDrink::Consume(Player* player) {
    if (!player) return;
    player->AddKarma(kKarmaBonus);
    isActive_ = false;
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        position_,
        nccu::gfx::Colors::White,
        "喝完飲料，精神好多了。下次小考應該能撐住。"
    });
}
