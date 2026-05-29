#ifndef ITEM_H_
#define ITEM_H_
#include "engine/core/GameObject.h"
#include <string>

/**
 * @file Item.h
 * @brief 所有可拾取道具的抽象基底：持有名稱與可拾取旗標，並定義 OnPickup 拾取鉤子。
 */

/**
 * @brief 地圖上可被玩家拾取之物件的共同基底。
 *
 * 在 GameObject 之上加入「名稱 + 是否可拾取」的最小道具語意，並以純虛擬
 * OnPickup() 作為各葉類別自訂拾取效果的擴充點（雨傘、消耗品、金錢、任務旗標
 * 等皆由此衍生），讓拾取流程對所有道具一致而效果各異。
 */
class Item : public GameObject {
public:
    /**
     * @brief 以世界座標、碰撞盒與名稱建構道具，預設為可拾取。
     * @param position 世界座標位置（像素）。
     * @param hitBox   軸對齊碰撞盒（AABB）。
     * @param name     道具識別名稱（亦作為背包 itemId）。
     */
    Item(nccu::engine::math::Vec2 position, nccu::engine::math::Rect hitBox, std::string name)
        : GameObject(position, hitBox), itemName_(std::move(name)), isPickable_(true) {}

    /**
     * @brief 玩家拾取此道具時觸發的效果鉤子。
     * @param player 拾取者。
     */
    virtual void OnPickup(Player* player) = 0;

    /** @brief 取得道具名稱。@return 名稱字串常參。 */
    [[nodiscard]] const std::string& GetName() const noexcept { return itemName_; }
    /** @brief 是否仍可被拾取。@return 可拾取回傳 true。 */
    [[nodiscard]] bool IsPickable() const noexcept { return isPickable_; }

protected:
    std::string itemName_;   ///< 道具名稱（兼作背包 itemId）
    bool isPickable_;        ///< 可拾取旗標
};

#endif // ITEM_H_
