#include "ProfessorTrapUmbrella.h"
#include "Player.h"
#include "EventBus.h"

void ProfessorTrapUmbrella::beClaimed(Player* player) {
    if (!player) return;
    player->SetHasUmbrella(true);
    spawnedEnemiesCount_ = 3; // simulated: TA NPCs to be spawned by GameWorld on this event
    isActive_ = false;
    EventBus::Instance().Publish(Event{
        EventType::UmbrellaClaimed,
        position_,
        umbrellaTint_,
        "ProfessorTrapUmbrella"
    });
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        position_,
        nccu::gfx::Colors::White,
        "你撿到了 ProfessorTrapUmbrella，遠處傳來助教們的腳步聲！"
    });
}
