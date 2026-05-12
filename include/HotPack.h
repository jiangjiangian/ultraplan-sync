#pragma once
#include "ConsumableItem.h"

// HotPack: warms the player up — dries off accumulated rain (rainMeter -> 0)
// and gives a small karma bump. Single-use; deactivates after Consume().
class HotPack : public ConsumableItem {
public:
    static constexpr int kPrice = 30;
    static constexpr int kKarmaBonus = 5;

    explicit HotPack(nccu::gfx::Vec2 position)
        : ConsumableItem(position, "HotPack", kPrice) {}

    void Consume(Player* player) override;
};
