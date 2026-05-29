#include "game/entities/EnergyDrink.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"

void EnergyDrink::Consume(Player* player) {
    if (!player) return;
    // 加業力並小幅烘乾雨量（-15）。與 ApplyConsumableEffect("EnergyDrink") 保持一致，
    // 兩條路徑由測試共同固定。
    player->AddKarma(kKarmaBonus).DrainRainBy(kRainRelief);
    isActive_ = false;
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "喝完飲料，精神好多了，淋到的雨也擦乾了一些。" });
}
