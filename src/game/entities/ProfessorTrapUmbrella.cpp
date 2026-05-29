#include "game/entities/ProfessorTrapUmbrella.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "game/quest/Flags.h"

void ProfessorTrapUmbrella::BeClaimed(Player* player) {
    if (!player) return;
    if (!isActive_) return;        // 冪等：第二次呼叫為無動作（與其他姊妹傘相同的守衛）
    player->SetHeldUmbrella(HeldUmbrella::ProfessorTrap);  // 記錄握傘種類並開啟遮蔽
    // 漣漪因子：Ch1 偷教授傘是 Ch2/Ch3/Ch4 兌現的因——助教 -10／Ch3 -10／Ch4 對峙 -15。
    // 若此處不設立此旗標，整條負面漣漪鏈將無從觸發（惡因無源）。置於冪等守衛之內以保證
    // 恰好生效一次。
    player->SetFlag(nccu::kFlagHasProfessorTrap);
    spawnedEnemiesCount_ = 3; // 模擬：由 GameWorld 於此事件生成的助教 NPC
    isActive_ = false;
    nccu::events::Sink().Publish(Event{ EventType::UmbrellaClaimed, "ProfessorTrapUmbrella" });
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "你撿到了 ProfessorTrapUmbrella，遠處傳來助教們的腳步聲！" });
}
