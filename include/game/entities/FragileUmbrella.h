#ifndef FRAGILE_UMBRELLA_H_
#define FRAGILE_UMBRELLA_H_
#include "game/entities/TransparentUmbrella.h"

/**
 * @file FragileUmbrella.h
 * @brief 破傘葉類別：以 BeClaimed 覆寫設立持傘狀態，並持有漏雨速率。
 */

/**
 * @brief 廉價、瀕臨失效的借來雨傘，雨傘 Template Method 樹的具體葉類別之一。
 *
 * 以泛白骨灰色搭配小巧 Broken 傘面（殘破傘骨），讀來像把廉價、快壞掉的借傘。
 */
class FragileUmbrella final : public TransparentUmbrella {
public:
    /** @brief 在指定世界座標生成破傘（固定骨灰色 Broken 外形）。@param[in] position 世界座標位置（像素）。 */
    explicit FragileUmbrella(nccu::engine::math::Vec2 position)
        : TransparentUmbrella(position, "FragileUmbrella",
                              nccu::engine::math::Color{210, 205, 190, 255},
                              UmbrellaStyle::Broken),
          leakRate_(0.5f) {}

    /**
     * @brief 認領破傘：設立持傘狀態（held-kind 與遮蔽）。
     * @param[in] player 認領者。
     */
    void BeClaimed(Player* player) override;

    /** @brief 取得漏雨速率。@return 每秒滲入的雨量比例。 */
    [[nodiscard]] float GetLeakRate() const noexcept { return leakRate_; }

private:
    float leakRate_;   ///< 漏雨速率
};

#endif // FRAGILE_UMBRELLA_H_
