#ifndef ENERGY_DRINK_H_
#define ENERGY_DRINK_H_
#include "entities/ConsumableItem.h"

// EnergyDrink: small karma bump representing the morale boost before an exam.
// G4: also dries off a small chunk of rain on use. Single-use; deactivates
// after Consume().
class EnergyDrink final : public ConsumableItem {
public:
    static constexpr int kPrice = 40;
    static constexpr int kKarmaBonus = 3;
    // G4: -15 rain units on use. Modest — its headline jobs are the karma
    // bump and waking the Ch2 學霸 (TryRescueBookworm consumes one).
    static constexpr float kRainRelief = 15.0f;

    explicit EnergyDrink(nccu::gfx::Vec2 position)
        : ConsumableItem(position, "EnergyDrink", kPrice) {}

    void Consume(Player* player) override;
};

#endif // ENERGY_DRINK_H_
