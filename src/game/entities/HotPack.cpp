#include "game/entities/HotPack.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"

void HotPack::Consume(Player* player) {
    if (!player) return;
    // 雨量減免從原本的歸零（resetRainMeter）改為固定 -25，使雨量支柱保有意義。與
    // ApplyConsumableEffect("HotPack") 保持一致，兩條路徑由測試共同固定。
    player->AddKarma(kKarmaBonus).DrainRainBy(kRainRelief);
    isActive_ = false;
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "用了暖暖包，烘乾了大半的雨水，心情也好了一些。" });
}
