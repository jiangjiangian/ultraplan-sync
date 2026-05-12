#include "CashPickup.h"
#include "EventBus.h"
#include "Player.h"
#include "gfx/Color.h"
#include "gfx/Rect.h"

#include <string>

CashPickup::CashPickup(nccu::gfx::Vec2 position, int value)
    : Item(position,
           // 16x16 hitbox anchored at the world position — same size as
           // ConsumableItem so it picks up via the same collision sweep.
           nccu::gfx::Rect{position.x, position.y, 16.0f, 16.0f},
           "Cash"),
      value_(value) {}

void CashPickup::OnPickup(Player* player) {
    // Null player is a no-op rather than a crash — Map Manager occasionally
    // sweeps pickups outside the Update loop, and we'd rather log nothing
    // than dereference garbage.
    if (!player) return;

    player->AddMoney(value_);
    isActive_ = false;

    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        position_,
        nccu::gfx::Colors::Yellow,
        std::string("撿到 ") + std::to_string(value_) + " 元"
    });
}
