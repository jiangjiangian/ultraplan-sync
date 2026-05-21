#include "entities/WaterproofSpray.h"
#include "entities/Player.h"
#include "controller/EventBus.h"

void WaterproofSpray::Consume(Player* player) {
    if (!player) return;
    // Persistent rain-immunity is a future-phase feature; for now the spray
    // is a mood-only consumable to keep this commit focused.
    isActive_ = false;
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "噴了防水噴霧，接下來這場雨就無感了。" });
}
