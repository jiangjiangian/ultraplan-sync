#include "entities/EnergyDrink.h"
#include "entities/Player.h"
#include "controller/EventBus.h"

void EnergyDrink::Consume(Player* player) {
    if (!player) return;
    player->AddKarma(kKarmaBonus);
    isActive_ = false;
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "喝完飲料，精神好多了。下次小考應該能撐住。" });
}
