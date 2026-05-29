#include "ui/world/QuestGiverIndicators.h"

#include "game/controller/GameObjectQueries.h"
#include "engine/core/GameObject.h"
#include "game/entities/Player.h"
#include "game/quest/QuestIndicator.h"
#include "game/state/SemesterState.h"
#include "ui/QuestGiverIndicator.h"
#include "game/world/World.h"
#include "game/world/WorldConfig.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"

namespace nccu {

void DrawQuestGiverIndicators(nccu::gfx::IRenderer& r, const World& world) {
    using namespace nccu::gfx;
    using nccu::queries::ForEachActive;

    // Quest-giver "!" overlay (H4). Drawn AFTER the painter's-order
    // pass but still inside the CameraScope so the icon follows the
    // NPC in world space — and on top of buildings/sprites that might
    // otherwise occlude a quest-giver tucked behind a footprint.
    // QuestIndicatorVisible (quest layer) is the SINGLE decision point:
    // it folds the roster's virtual IsQuestGiver() bit together with
    // the per-chapter rules (Ch3 sequential A→B→C chain, Item 4a head
    // light; Ch4 finale 助教-only `!`, Item 1b) so the View stays
    // pure-render — no gameplay logic, no dynamic_cast. NB the Ch4
    // finale NPC is isQuestGiver=false in the roster, so the decision
    // can NO LONGER short-circuit on IsQuestGiver() alone; the predicate
    // owns it. QuestGiverIndicator routes every primitive through
    // IRenderer, keeping the helper headless-testable.
    const Player* qgPlayer = world.GetPlayer();
    if (!qgPlayer) return;
    const nccu::SemesterState qgState = world.Semester().Current();
    ForEachActive(world.Objects(), [&](const GameObject& o) {
        if (!nccu::QuestIndicatorVisible(o.NpcId(), o.IsQuestGiver(),
                                         qgState, *qgPlayer)) return;
        // The hit box lives at the NPC's feet; QuestGiverIndicator
        // lifts the "!" above the bottom-anchored sprite top.
        DrawQuestGiverIndicator(
            r,
            Rect{o.GetPosition().x, o.GetPosition().y,
                 ::world::kPlayerWidth, ::world::kPlayerHeight});
    });
}

}  // namespace nccu
