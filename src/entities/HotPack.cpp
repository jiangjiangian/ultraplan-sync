#include "entities/HotPack.h"
#include "entities/Player.h"
#include "engine/events/EventBus.h"

void HotPack::Consume(Player* player) {
    if (!player) return;
    // G4 rebalance: was resetRainMeter() (full dry); now a fixed -25 so the
    // rain pillar stays meaningful. Kept in lockstep with
    // ApplyConsumableEffect("HotPack") (a doctest pins both paths).
    player->AddKarma(kKarmaBonus).DrainRainBy(kRainRelief);
    isActive_ = false;
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "用了暖暖包，烘乾了大半的雨水，心情也好了一些。" });
}
