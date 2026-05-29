#ifndef TRUE_UMBRELLA_H_
#define TRUE_UMBRELLA_H_
#include "game/entities/TransparentUmbrella.h"

/**
 * @file TrueUmbrella.h
 * @brief 真傘葉類別：以 BeClaimed 覆寫設立通往 Ending A 的持傘條件。
 */

/**
 * @brief 玩家要尋回的「正確」雨傘，雨傘 Template Method 樹的具體葉類別之一。
 *
 * 以明亮天藍色搭配完整 Domed 傘面，給出毫不含糊「這是乾淨／正確的傘」的視覺印象。
 */
class TrueUmbrella final : public TransparentUmbrella {
public:
    /** @brief 在指定世界座標生成真傘（固定藍色 Domed 外形）。@param[in] position 世界座標位置（像素）。 */
    explicit TrueUmbrella(nccu::engine::math::Vec2 position)
        : TransparentUmbrella(position, "TrueUmbrella",
                              nccu::engine::math::Color{70, 190, 255, 255},
                              UmbrellaStyle::Domed) {}

    /**
     * @brief 認領真傘：設立持傘狀態與 TrueUmbrella 專屬旗標，並發出對應事件。
     * @param[in] player 認領者。
     */
    void BeClaimed(Player* player) override;
};

#endif // TRUE_UMBRELLA_H_
