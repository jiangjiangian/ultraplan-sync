#pragma once
#include "ConsumableItem.h"

// WaterproofSpray: placeholder mood-only consumable. The persistent
// rain-immunity effect is a future-phase feature; for now Consume() only
// publishes a flavor message and deactivates the item.
class WaterproofSpray : public ConsumableItem {
public:
    static constexpr int kPrice = 50;

    explicit WaterproofSpray(nccu::gfx::Vec2 position)
        : ConsumableItem(position, "WaterproofSpray", kPrice) {}

    void Consume(Player* player) override;
};
