#include "game/entities/FragileUmbrella.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"

void FragileUmbrella::beClaimed(Player* player) {
    if (!player) return;
    if (!isActive_) return;        // idempotent: a second call is a no-op
    player->SetHeldUmbrella(HeldUmbrella::Fragile);  // B2.1: held-kind + shelter
    isActive_ = false;
    nccu::events::Sink().Publish(Event{ EventType::UmbrellaClaimed, "FragileUmbrella" });
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "你撿到了 FragileUmbrella，骨架斷了，雨還是會慢慢滲進來。" });
}
