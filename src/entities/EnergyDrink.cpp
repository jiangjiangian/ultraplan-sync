#include "entities/EnergyDrink.h"
#include "entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"

void EnergyDrink::Consume(Player* player) {
    if (!player) return;
    // G4: karma bump AND a small rain dry (-15). Kept in lockstep with
    // ApplyConsumableEffect("EnergyDrink") (a doctest pins both paths).
    player->AddKarma(kKarmaBonus).DrainRainBy(kRainRelief);
    isActive_ = false;
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "喝完飲料，精神好多了，淋到的雨也擦乾了一些。" });
}
