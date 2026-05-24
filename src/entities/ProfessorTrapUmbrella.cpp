#include "entities/ProfessorTrapUmbrella.h"
#include "entities/Player.h"
#include "controller/EventBus.h"

void ProfessorTrapUmbrella::beClaimed(Player* player) {
    if (!player) return;
    if (spawnedEnemiesCount_ > 0) return;        // idempotent: a second call is a no-op
    player->SetHeldUmbrella(HeldUmbrella::ProfessorTrap);  // B2.1: held-kind + shelter
    // Ripple seed (F.9-a): Ch1 偷教授傘 is the cause Ch2/Ch3/Ch4 cash in
    // — 助教 (c) -10 / Ch3 -10 / Ch4 對峙 -15. Without setting this here
    // the whole negative-ripple chain was unreachable (惡因無源). Inside
    // the idempotent guard so it lands exactly once.
    player->SetFlag("Flag_HasProfessorTrap");
    spawnedEnemiesCount_ = 3; // simulated: TA NPCs to be spawned by GameWorld on this event
    isActive_ = false;
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "ProfessorTrapUmbrella" });
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "你撿到了 ProfessorTrapUmbrella，遠處傳來助教們的腳步聲！" });
}
