#include "entities/TransparentUmbrella.h"
#include "engine/events/EventBus.h"
#include "entities/Player.h"
#include "quest/Flags.h"
#include "engine/render/IRenderer.h"
#include "gfx/UmbrellaGlyph.h"

#include <string>

namespace {
// Shared by both pick-up entry points. The main quest must be active
// (the player accepted the 苦主's plea — Flag_PromisedVictim) before any
// umbrella can be claimed; otherwise nudge the player toward the quest
// rather than doing nothing (a silent no-op reads as a broken pickup).
bool QuestGateOpen(Player* player) {
    if (!player) return true;                  // preserve prior null path
    if (player->HasFlag(nccu::kFlagPromisedVictim)) return true;
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
    // The owner pinned each umbrella's look (真傘=藍 / 破傘=剩手柄 /
    // 詛咒傘=暗紫 / 醜傘=綠, ProfessorTrap a danger-red trap). All four
    // silhouettes + their signature colours live in ONE place,
    // gfx::DrawUmbrellaGlyph, which is ALSO what the ground pickup and the
    // ending card draw — so a given umbrella reads identically wherever it
    // appears. The per-subclass UmbrellaStyle just selects which look; the
    // colour now comes from the shared glyph (umbrellaTint_ is retained on
    // the object for any caller that wants the tint, but Render no longer
    // needs it). Rect-only, no sprite/text — MVC clean (the View renders
    // off the object's own data; no sim/state touched).
    nccu::gfx::DrawUmbrellaGlyph(renderer, LookForStyle(style_),
                                 nccu::gfx::Rect{position_.x, position_.y,
                                                 20.0f, 20.0f});
}
