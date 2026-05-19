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
    // REQUIREMENT #9: a distinct silhouette PER umbrella style inside the
    // 20x20 footprint (all rect-only — IRenderer has no circle/triangle),
    // so the three Ch1 choices read apart at a glance even at map scale
    // and on displays where the tints alone are subtle. Template Method —
    // the skeleton lives here; each subclass supplies its bold tint AND
    // its UmbrellaStyle via the constructor. MVC clean: pure render off
    // the object's own data, no sim/state touched.
    using nccu::gfx::Rect;
    namespace C = nccu::gfx::Colors;
    const float x = position_.x;
    const float y = position_.y;
    const nccu::gfx::Color t = umbrellaTint_;

    switch (style_) {
        case UmbrellaStyle::Domed: {
            // True — a wide, full, rounded 3-tier canopy: the "complete /
            // correct" read. Light handle so the clear umbrella looks
            // intact and tidy.
            renderer.DrawRect(Rect{x + 6.0f, y + 2.0f,  8.0f, 2.0f}, t);
            renderer.DrawRect(Rect{x + 3.0f, y + 4.0f, 14.0f, 3.0f}, t);
            renderer.DrawRect(Rect{x + 0.0f, y + 7.0f, 20.0f, 3.0f}, t);
            renderer.DrawRect(Rect{x + 9.0f, y + 10.0f, 2.0f, 9.0f}, C::DarkGray);
            renderer.DrawRect(Rect{x + 7.0f, y + 18.0f, 6.0f, 2.0f}, C::DarkGray);
            break;
        }
        case UmbrellaStyle::Broken: {
            // Fragile — a small, narrow canopy with a missing right rib
            // (the gap) and a bent handle: reads as cheap / about to fail.
            renderer.DrawRect(Rect{x + 5.0f, y + 5.0f, 8.0f, 3.0f}, t);
            renderer.DrawRect(Rect{x + 3.0f, y + 8.0f, 9.0f, 3.0f}, t); // short — torn rib gap on the right
            renderer.DrawRect(Rect{x + 14.0f, y + 8.0f, 3.0f, 2.0f}, t); // detached rib tip
            renderer.DrawRect(Rect{x + 8.0f, y + 11.0f, 2.0f, 6.0f}, C::DarkGray);
            renderer.DrawRect(Rect{x + 8.0f, y + 17.0f, 4.0f, 2.0f}, C::DarkGray); // bent foot
            break;
        }
        case UmbrellaStyle::Spiked: {
            // ProfessorTrap — an angular STEPPED pyramid canopy with a
            // sharp tip: an alarming, weaponised silhouette (the trap).
            renderer.DrawRect(Rect{x + 9.0f, y + 1.0f,  2.0f, 3.0f}, t);  // spike tip
            renderer.DrawRect(Rect{x + 7.0f, y + 4.0f,  6.0f, 2.0f}, t);
            renderer.DrawRect(Rect{x + 4.0f, y + 6.0f, 12.0f, 2.0f}, t);
            renderer.DrawRect(Rect{x + 1.0f, y + 8.0f, 18.0f, 2.0f}, t);
            // jagged barbs at the canopy hem
            renderer.DrawRect(Rect{x + 2.0f, y + 10.0f, 2.0f, 2.0f}, t);
            renderer.DrawRect(Rect{x + 9.0f, y + 10.0f, 2.0f, 2.0f}, t);
            renderer.DrawRect(Rect{x + 16.0f, y + 10.0f, 2.0f, 2.0f}, t);
            renderer.DrawRect(Rect{x + 9.0f, y + 12.0f, 2.0f, 7.0f}, C::DarkGray);
            break;
        }
        case UmbrellaStyle::Drooping: {
            // Cursed — a sagging, heavy canopy whose ribs hang DOWN at the
            // sides, plus a pure-black handle: an oppressive "wrong" read.
            renderer.DrawRect(Rect{x + 7.0f, y + 3.0f,  6.0f, 2.0f}, t);
            renderer.DrawRect(Rect{x + 3.0f, y + 5.0f, 14.0f, 3.0f}, t);
            renderer.DrawRect(Rect{x + 1.0f, y + 8.0f, 18.0f, 2.0f}, t);
            renderer.DrawRect(Rect{x + 1.0f, y + 10.0f, 3.0f, 4.0f}, t);  // left rib drooping down
            renderer.DrawRect(Rect{x + 16.0f, y + 10.0f, 3.0f, 4.0f}, t); // right rib drooping down
            renderer.DrawRect(Rect{x + 9.0f, y + 10.0f, 2.0f, 9.0f}, C::Black);
            break;
        }
    }
}
