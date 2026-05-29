#include "game/entities/FragileUmbrella.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"

void FragileUmbrella::BeClaimed(Player* player) {
    if (!player) return;
    if (!isActive_) return;        // 冪等：第二次呼叫為無動作
    player->SetHeldUmbrella(HeldUmbrella::Fragile);  // 記錄握傘種類並開啟遮蔽
    isActive_ = false;
    nccu::events::Sink().Publish(Event{ EventType::UmbrellaClaimed, "FragileUmbrella" });
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "你撿到了 FragileUmbrella，骨架斷了，雨還是會慢慢滲進來。" });
}
