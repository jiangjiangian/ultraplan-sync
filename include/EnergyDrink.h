#ifndef ENERGY_DRINK_H_
#define ENERGY_DRINK_H_
#include "ConsumableItem.h"

// EnergyDrink: small karma bump representing the morale boost before an exam.
// Single-use; deactivates after Consume().
class EnergyDrink final : public ConsumableItem {
public:
    static constexpr int kPrice = 40;
    static constexpr int kKarmaBonus = 3;

    explicit EnergyDrink(nccu::gfx::Vec2 position)
        : ConsumableItem(position, "EnergyDrink", kPrice) {}

    void Consume(Player* player) override;
};

#endif // ENERGY_DRINK_H_
