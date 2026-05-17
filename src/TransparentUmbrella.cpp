#include "TransparentUmbrella.h"
#include "EventBus.h"
#include "Player.h"
#include "gfx/IRenderer.h"

#include <string>

namespace {
// Shared by both pick-up entry points. The main quest must be active
// (the player accepted the 苦主's plea — Flag_PromisedVictim) before any
// umbrella can be claimed; otherwise nudge the player toward the quest
// rather than doing nothing (a silent no-op reads as a broken pickup).
bool QuestGateOpen(Player* player) {
    if (!player) return true;                  // preserve prior null path
    if (player->HasFlag("Flag_PromisedVictim")) return true;
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        std::string("這把傘不是你的——先去找那位掉了傘的同學問問吧。")});
    return false;
}
}  // namespace

void TransparentUmbrella::Interact(Player* initiator) {
    if (QuestGateOpen(initiator)) beClaimed(initiator);
}

void TransparentUmbrella::OnPickup(Player* player) {
    if (QuestGateOpen(player)) beClaimed(player);
}

void TransparentUmbrella::Render(nccu::gfx::IRenderer& renderer) const {
    // 3-rect umbrella glyph inside the item's 20x20 footprint: a tapered
    // canopy in umbrellaTint_ plus a dark handle. Template Method — the
    // skeleton lives here; the four subclasses only supply their distinct
    // tint via the constructor, so they read at a glance on the map.
    const float x = position_.x;
    const float y = position_.y;
    renderer.DrawRect(nccu::gfx::Rect{x + 2.0f, y +  4.0f, 16.0f, 3.0f}, umbrellaTint_);
    renderer.DrawRect(nccu::gfx::Rect{x + 0.0f, y +  7.0f, 20.0f, 3.0f}, umbrellaTint_);
    renderer.DrawRect(nccu::gfx::Rect{x + 9.0f, y + 10.0f,  2.0f, 9.0f},
                      nccu::gfx::Colors::DarkGray);
}
