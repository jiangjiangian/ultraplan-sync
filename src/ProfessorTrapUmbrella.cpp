#include "ProfessorTrapUmbrella.h"
#include "Player.h"
#include "EventBus.h"

void ProfessorTrapUmbrella::beClaimed(Player* player) {
    if (!player) return;
    if (spawnedEnemiesCount_ > 0) return;        // idempotent: a second call is a no-op
    player->SetHasUmbrella(true);
    spawnedEnemiesCount_ = 3; // simulated: TA NPCs to be spawned by GameWorld on this event
    isActive_ = false;
    EventBus::Instance().Publish(Event{ EventType::UmbrellaClaimed, "ProfessorTrapUmbrella" });
    EventBus::Instance().Publish(Event{ EventType::ShowMessage, "你撿到了 ProfessorTrapUmbrella，遠處傳來助教們的腳步聲！" });
}
