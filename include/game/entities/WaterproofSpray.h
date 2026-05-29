#ifndef WATERPROOF_SPRAY_H_
#define WATERPROOF_SPRAY_H_
#include "game/entities/ConsumableItem.h"

// WaterproofSpray: the dedicated rain-relief consumable. G4: using it now
// sheds the biggest chunk of accumulated rain (-35 units) — a persistent
// rain-immunity buff is still a future-phase feature, but the spray is no
// longer mood-only. No karma effect (it is gear, not a kind act).
class WaterproofSpray : public ConsumableItem {
public:
    static constexpr int kPrice = 50;
    // G4: -35 rain units on use — the strongest single-use dry in the bag
    // (it IS the waterproofing item), but still below a full 100 reset.
    static constexpr float kRainRelief = 35.0f;

    explicit WaterproofSpray(nccu::engine::math::Vec2 position)
        : ConsumableItem(position, "WaterproofSpray", kPrice) {}

    void Consume(Player* player) override;
};

#endif // WATERPROOF_SPRAY_H_
