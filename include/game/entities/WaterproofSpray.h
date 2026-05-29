#ifndef WATERPROOF_SPRAY_H_
#define WATERPROOF_SPRAY_H_
#include "game/entities/ConsumableItem.h"

/**
 * @file WaterproofSpray.h
 * @brief 防水噴霧消耗品葉類別：專責雨量減免，單次抹去最大一塊雨量、不影響業力。
 */

/**
 * @brief 防水噴霧，ConsumableItem 的具體葉類別，是專責雨量減免的消耗品。
 *
 * 使用時抹去累積雨量中最大的一塊（-35 點）；持久的雨中免疫增益仍是未來階段的功能。
 * 不影響業力（它是裝備，而非善行）。
 */
class WaterproofSpray : public ConsumableItem {
public:
    static constexpr int kPrice = 50;        ///< 攤販售價
    /// 使用時減 35 點雨量——背包中單次烘乾最強者（它本就是防水道具），但仍低於完全歸零（100）。
    static constexpr float kRainRelief = 35.0f;

    /** @brief 在指定世界座標生成防水噴霧。@param[in] position 世界座標位置（像素）。 */
    explicit WaterproofSpray(nccu::engine::math::Vec2 position)
        : ConsumableItem(position, "WaterproofSpray", kPrice) {}

    /**
     * @brief 施放防水噴霧效果：大幅減少雨量（不影響業力）。
     * @param[in] player 使用者。
     */
    void Consume(Player* player) override;
};

#endif // WATERPROOF_SPRAY_H_
