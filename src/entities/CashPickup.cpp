#include "entities/CashPickup.h"
#include "controller/EventBus.h"
#include "entities/Player.h"
#include "engine/math/Color.h"
#include "gfx/IRenderer.h"
#include "engine/math/Rect.h"

#include <string>

void CashPickup::Render(nccu::gfx::IRenderer& renderer) const {
    // Visible ground marker so loose cash is spottable while exploring —
    // an invisible pickup is undiscoverable (the exact gap 1b-5 fixed for
    // QuestFlagPickup). The full-hitbox Green block read as anonymous
    // debris in playtest ("一堆綠色方塊，不知道是啥"); a small centred
    // GOLD token reads as a dropped coin (gold == money is universal) and
    // stays distinct from the Yellow quest marker. DrawRect only — Item
    // must not call DrawText/DrawTexture (architecture rule); no raylib.
    constexpr float kCoin = 10.0f;                 // < the 16px hitbox
    const float inset = (hitBox_.width - kCoin) * 0.5f;
    renderer.DrawRect(
        nccu::gfx::Rect{hitBox_.x + inset, hitBox_.y + inset, kCoin, kCoin},
        nccu::gfx::Colors::Gold);
}

CashPickup::CashPickup(nccu::gfx::Vec2 position, int value)
    // Direct base is WithRoles<CashPickup, Item>; its `using Base::Base`
    // inherits Item's ctor so this 3-arg form still resolves.
    : WithRoles(position,
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

    EventBus::Instance().Publish(Event{ EventType::ShowMessage, std::string("撿到 ") + std::to_string(value_) + " 元" });
}
