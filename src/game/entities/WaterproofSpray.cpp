#include "game/entities/WaterproofSpray.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"

void WaterproofSpray::Consume(Player* player) {
    if (!player) return;
    // 專責雨量減免的道具——單次抹去累積雨量中最大的一塊（-35）。不影響業力（它是裝備，
    // 而非善行）。持久的雨中免疫仍是未來階段的功能。與 ApplyConsumableEffect("WaterproofSpray")
    // 保持一致。
    player->DrainRainBy(kRainRelief);
    isActive_ = false;
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "噴了防水噴霧，雨水大半都被彈開了。" });
}
