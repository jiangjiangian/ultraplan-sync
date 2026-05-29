#ifndef QUEST_FLAG_PICKUP_H_
#define QUEST_FLAG_PICKUP_H_
#include "game/entities/Item.h"
#include "engine/math/Vec2.h"
#include <string>
#include <vector>

// A one-shot ground item that sets a named Player flag on pickup, then
// deactivates (end-of-frame sweep removes it). Quest-chain plumbing —
// e.g. the 四維道 form that unlocks the 助教 reward, or a Ch2 散落筆記.
// Draws a small ground marker so the player can spot and collect it
// while exploring; picked up via the E-interact sweep (empty NpcId() ->
// Interact()).
//
// Optional set-completion reward (S5c-2): a pickup may carry a list of
// sibling flags + a karma bonus. When THIS pickup's flag set leaves the
// whole list satisfied, the bonus is granted — used for 學霸 (b)'s
// `// karma +3` once all three notes are in. Only the pickup collected
// LAST sees every flag set (earlier ones skip; collected ones are gone),
// so the bonus fires exactly once with no guard flag.
//
// Optional COUNT-BASED messages: when countMessages_ is non-empty, the
// pickup's on-screen line is chosen by HOW MANY of completionFlags_ the
// player now holds (1st collected -> [0], 2nd -> [1], 3rd -> [2]), NOT by
// which specific item this is — so picking the notes in ANY order prints
// "1st found / 2nd found / last found" correctly (the bug was an identity
// message that said "last page" whenever note3 was grabbed first). When
// countMessages_ is empty the single message_ is used (the 申請書 / any
// non-set pickup keeps its exact prior behaviour).
// ISP roles: IDrawable + IInteractable. The old Update body was an empty
// no-op (the form doesn't tick), so that role is dropped; the Yellow
// ground-marker Render and the flag-setting OnPickup (via Interact) are
// real and kept. A leaf, so WithRoles is keyed on QuestFlagPickup itself.
class QuestFlagPickup final : public WithRoles<QuestFlagPickup, Item>,
                              public IDrawable, public IInteractable {
public:
    QuestFlagPickup(nccu::engine::math::Vec2 position, std::string flagName,
                    std::string message = "撿到了被風吹走的申請書",
                    std::vector<std::string> completionFlags = {},
                    int completionKarma = 0,
                    std::vector<std::string> countMessages = {});

    void Render(nccu::engine::render::IRenderer& renderer) const override;
    void Interact(Player* initiator) override { OnPickup(initiator); }
    void OnPickup(Player* player) override;

private:
    std::string              flagName_;
    std::string              message_;
    std::vector<std::string> completionFlags_;
    int                      completionKarma_;
    std::vector<std::string> countMessages_;
};
#endif // QUEST_FLAG_PICKUP_H_
