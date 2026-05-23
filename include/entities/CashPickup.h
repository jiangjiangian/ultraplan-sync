#ifndef CASH_PICKUP_H_
#define CASH_PICKUP_H_
#include "entities/Item.h"
#include "gfx/Vec2.h"

// CashPickup: a one-shot ground item that converts its `value_` field into
// the player's money pool on collision. Mirrors the consumable-item shape
// but lives in the Item subtree directly because there is no Consume()
// semantics to share — money pickups have no animation / no karma delta.
// ISP roles: IDrawable + IInteractable. The old Update body was an empty
// no-op (cash doesn't tick), so that role is dropped; the Gold-token
// Render and the on-collision OnPickup (via Interact) are real and kept.
// A leaf, so WithRoles is keyed on CashPickup itself.
class CashPickup final : public WithRoles<CashPickup, Item>,
                         public IDrawable, public IInteractable {
public:
    CashPickup(nccu::gfx::Vec2 position, int value);

    void Render(nccu::gfx::IRenderer& renderer) const override;  // visible ground marker
    void Interact(Player* initiator) override { OnPickup(initiator); }

    void OnPickup(Player* player) override;

    [[nodiscard]] int Value() const noexcept { return value_; }

private:
    int value_;
};

#endif // CASH_PICKUP_H_
