#ifndef HOT_PACK_H_
#define HOT_PACK_H_
#include "game/entities/ConsumableItem.h"

// HotPack: warms the player up — dries off accumulated rain and gives a
// small karma bump. Single-use; deactivates after Consume().
// G4 rebalance: was a FULL dry (rainMeter -> 0); now a fixed -25 units so
// the rain pillar stays meaningful even with consumables in the bag (one
// 暖暖包 no longer wipes the whole meter — the owner's "rain must stay
// meaningful, game stays winnable").
class HotPack final : public ConsumableItem {
public:
    static constexpr int kPrice = 30;
    static constexpr int kKarmaBonus = 5;
    // G4: -25 rain units on use (the strongest food-tier dry; still not a
    // full reset).
    static constexpr float kRainRelief = 25.0f;

    explicit HotPack(nccu::engine::math::Vec2 position)
        : ConsumableItem(position, "HotPack", kPrice) {}

    void Consume(Player* player) override;
};

#endif // HOT_PACK_H_
