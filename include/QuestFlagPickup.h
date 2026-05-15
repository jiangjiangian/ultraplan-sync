#ifndef QUEST_FLAG_PICKUP_H_
#define QUEST_FLAG_PICKUP_H_
#include "Item.h"
#include "gfx/Vec2.h"
#include <string>

// A one-shot ground item that sets a named Player flag on pickup, then
// deactivates (end-of-frame sweep removes it). Quest-chain plumbing —
// e.g. the 四維道 form that unlocks the 助教 reward. Mirrors CashPickup:
// pure-data, nothing to draw, picked up via the E-interact sweep
// (empty NpcId() -> Interact()).
class QuestFlagPickup final : public Item {
public:
    QuestFlagPickup(nccu::gfx::Vec2 position, std::string flagName);

    void Update(float /*deltaTime*/) override {}
    void Render(nccu::gfx::IRenderer&) const override {}
    void Interact(Player* initiator) override { OnPickup(initiator); }
    void OnPickup(Player* player) override;

private:
    std::string flagName_;
};
#endif // QUEST_FLAG_PICKUP_H_
