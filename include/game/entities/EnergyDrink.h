#ifndef ENERGY_DRINK_H_
#define ENERGY_DRINK_H_
#include "game/entities/ConsumableItem.h"

/**
 * @file EnergyDrink.h
 * @brief 提神飲料消耗品葉類別：使用時小幅加業力並擦乾少量雨量。
 */

/**
 * @brief 提神飲料，ConsumableItem 的具體葉類別，象徵考前的士氣提振。
 *
 * 使用時小幅加業力並擦乾一小塊雨量；單次使用，Consume() 後失效。
 */
class EnergyDrink final : public ConsumableItem {
public:
    static constexpr int kPrice = 40;        ///< 攤販售價
    static constexpr int kKarmaBonus = 3;    ///< 使用時的業力加成
    /// 使用時減 15 點雨量。幅度溫和——主要任務是加業力與喚醒 Ch2 學霸（TryRescueBookworm 會消耗一瓶）。
    static constexpr float kRainRelief = 15.0f;

    /** @brief 在指定世界座標生成提神飲料。@param[in] position 世界座標位置（像素）。 */
    explicit EnergyDrink(nccu::engine::math::Vec2 position)
        : ConsumableItem(position, "EnergyDrink", kPrice) {}

    /**
     * @brief 施放提神飲料效果：加業力並減少雨量。
     * @param[in] player 使用者。
     */
    void Consume(Player* player) override;
};

#endif // ENERGY_DRINK_H_
