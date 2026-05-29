#ifndef UI_WORLD_QUEST_GIVER_INDICATORS_H_
#define UI_WORLD_QUEST_GIVER_INDICATORS_H_

namespace nccu {

class World;
namespace engine::render { class IRenderer; }

// Quest-giver "!" overlays (P1 step 7f — extracted from
// View::RenderWorld). Iterates active NPCs and lifts the quest
// indicator above any whose `QuestIndicatorVisible(npcId,
// IsQuestGiver, semester, player)` predicate fires.
//
// Drawn AFTER the painter's-order pass but INSIDE the `CameraScope`
// the caller is holding, so the indicators follow each NPC in world
// space and sit ON TOP of buildings/sprites that might otherwise
// occlude a quest-giver tucked behind a footprint.
//
// `QuestIndicatorVisible` is the SINGLE decision point (folds the
// roster's `IsQuestGiver()` bit with per-chapter rules), so the View
// stays pure-render: no gameplay logic, no `dynamic_cast`. The Ch4
// finale NPC has `isQuestGiver=false`, so the predicate cannot
// short-circuit on `IsQuestGiver()` alone — `QuestIndicatorVisible`
// owns that.
//
// No-op when there is no player; safe to call every frame.
void DrawQuestGiverIndicators(nccu::engine::render::IRenderer& r, const World& world);

}  // namespace nccu

#endif  // UI_WORLD_QUEST_GIVER_INDICATORS_H_
