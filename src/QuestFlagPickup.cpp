#include "QuestFlagPickup.h"
#include "EventBus.h"
#include "Player.h"
#include "gfx/Rect.h"
#include <string>

QuestFlagPickup::QuestFlagPickup(nccu::gfx::Vec2 position,
                                 std::string flagName)
    : Item(position,
           nccu::gfx::Rect{position.x, position.y, 16.0f, 16.0f},
           "QuestItem"),
      flagName_(std::move(flagName)) {}

void QuestFlagPickup::OnPickup(Player* player) {
    if (!player) return;
    player->SetFlag(flagName_);
    isActive_ = false;
    EventBus::Instance().Publish(
        Event{ EventType::ShowMessage, std::string("撿到了被風吹走的申請書") });
}
