#ifndef HOT_PACK_H_
#define HOT_PACK_H_
#include "game/entities/ConsumableItem.h"

/**
 * @file HotPack.h
 * @brief 暖暖包消耗品葉類別：使用時烘乾大半雨量並小幅加業力。
 */

/**
 * @brief 暖暖包，ConsumableItem 的具體葉類別，讓玩家暖和起來。
 *
 * 使用時烘乾累積的雨量並小幅加業力；單次使用，Consume() 後失效。雨量減免為固定 -25
 * 點（而非歸零），使雨量這項支柱即便背包裡有消耗品仍保有意義——一個暖暖包不再抹平整條
 * 雨量條，遊戲仍可通關。
 */
class HotPack final : public ConsumableItem {
public:
    static constexpr int kPrice = 30;        ///< 攤販售價
    static constexpr int kKarmaBonus = 5;    ///< 使用時的業力加成
    /// 使用時減 25 點雨量（食物層級中最強的烘乾，但仍非完全歸零）。
    static constexpr float kRainRelief = 25.0f;

    /** @brief 在指定世界座標生成暖暖包。@param[in] position 世界座標位置（像素）。 */
    explicit HotPack(nccu::engine::math::Vec2 position)
        : ConsumableItem(position, "HotPack", kPrice) {}

    /**
     * @brief 施放暖暖包效果：減少雨量並加業力。
     * @param[in] player 使用者。
     */
    void Consume(Player* player) override;
};

#endif // HOT_PACK_H_
