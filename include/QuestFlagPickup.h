#ifndef QUEST_FLAG_PICKUP_H_
#define QUEST_FLAG_PICKUP_H_
#include "Item.h"
#include "gfx/Vec2.h"
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
class QuestFlagPickup final : public Item {
public:
    QuestFlagPickup(nccu::gfx::Vec2 position, std::string flagName,
                    std::string message = "撿到了被風吹走的申請書",
                    std::vector<std::string> completionFlags = {},
                    int completionKarma = 0);

    void Update(float /*deltaTime*/) override {}
    void Render(nccu::gfx::IRenderer& renderer) const override;
    void Interact(Player* initiator) override { OnPickup(initiator); }
    void OnPickup(Player* player) override;

private:
    std::string              flagName_;
    std::string              message_;
    std::vector<std::string> completionFlags_;
    int                      completionKarma_;
};
#endif // QUEST_FLAG_PICKUP_H_
