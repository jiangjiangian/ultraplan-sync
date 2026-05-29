#ifndef CASH_PICKUP_H_
#define CASH_PICKUP_H_
#include "game/entities/Item.h"
#include "engine/math/Vec2.h"

/**
 * @file CashPickup.h
 * @brief 金錢拾取物：碰撞時將自身 value_ 灌入玩家錢包的一次性地面道具。
 */

/**
 * @brief 一次性的地面金錢道具，碰撞時把 value_ 換成玩家的金錢。
 *
 * 外形與消耗品相仿，但因無 Consume() 語意可共用（金錢拾取無動畫、無業力增減），故直接
 * 落在 Item 子樹之下，而非走 ConsumableItem 中介層。
 *
 * ISP 角色：IDrawable + IInteractable。原本的 Update 為空殼（金錢不需逐幀更新），故捨棄
 * 該角色；金幣字符的 Render 與碰撞觸發（經 Interact）的 OnPickup 皆為實作而保留。葉類別，
 * 故 WithRoles 以 CashPickup 自身為鍵。
 */
class CashPickup final : public WithRoles<CashPickup, Item>,
                         public IDrawable, public IInteractable {
public:
    /**
     * @brief 以世界座標與金額建構金錢拾取物。
     * @param[in] position 世界座標位置（像素）。
     * @param[in] value    拾取時加入錢包的金額。
     */
    CashPickup(nccu::engine::math::Vec2 position, int value);

    /**
     * @brief 繪出可見的地面金幣標記。
     * @param[in,out] renderer 注入的渲染介面。
     */
    void Render(nccu::engine::render::IRenderer& renderer) const override;
    /** @brief 互動即拾取（轉呼叫 OnPickup）。@param[in] initiator 互動發起者（玩家）。 */
    void Interact(Player* initiator) override { OnPickup(initiator); }

    /**
     * @brief 拾取效果：把金額加入玩家錢包並使自身失效。
     * @param[in] player 拾取者。
     */
    void OnPickup(Player* player) override;

    /** @brief 取得金額。@return 此拾取物的面額。 */
    [[nodiscard]] int Value() const noexcept { return value_; }

private:
    int value_;   ///< 拾取時加入錢包的金額
};

#endif // CASH_PICKUP_H_
