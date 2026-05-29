#include "game/entities/WaterproofSpray.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"

void WaterproofSpray::Consume(Player* player) {
    if (!player) return;
    // G4: the dedicated rain-relief item — sheds the biggest single-use
    // chunk of accumulated rain (-35). No karma (it is gear, not a kind
    // act). Persistent rain-immunity remains a future-phase feature. Kept
    // in lockstep with ApplyConsumableEffect("WaterproofSpray").
    player->DrainRainBy(kRainRelief);
    isActive_ = false;
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "噴了防水噴霧，雨水大半都被彈開了。" });
}
