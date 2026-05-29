#ifndef CONSUMABLE_ITEM_H_
#define CONSUMABLE_ITEM_H_
#include "game/entities/Item.h"
#include "game/entities/Player.h"

/**
 * @file ConsumableItem.h
 * @brief 可消耗道具的抽象中間層：拾取入背包、使用時以 Consume() 多型施放效果。
 */

/**
 * @brief Item 與各具體消耗品之間的中間基底（Template Method 的抽象骨架）。
 *
 * 對應 TransparentUmbrella 在雨傘樹中的中介角色，差別在此處的多型動詞是
 * Consume() 而非 BeClaimed()。拾取流程（入背包）在本層定稿，效果則延後到
 * 從背包「使用」時才由各葉類別的 Consume() 施放。
 *
 * ISP 角色：僅 IInteractable。原本的 Update／Render 皆為空殼（無須更新，飲品
 * 繪製由 View 負責），故不再扮演那兩個角色，更新／繪製迴圈自然略過——行為相同
 * 而少了空覆寫。EnergyDrink／HotPack／WaterproofSpray 三個葉類別共用此角色集，
 * 因此 WithRoles 以本中間層為鍵（Derived = ConsumableItem）；每個葉類別皆
 * IS-A ConsumableItem，static_cast<ConsumableItem*> 因而合法。
 */
class ConsumableItem : public WithRoles<ConsumableItem, Item>, public IInteractable {
public:
    /**
     * @brief 以世界座標、名稱與售價建構消耗品（碰撞盒固定 16×16）。
     * @param position 世界座標位置（像素）。
     * @param name     道具名稱（兼作背包 itemId）。
     * @param price    在攤販處的售價。
     */
    ConsumableItem(nccu::engine::math::Vec2 position, std::string name, int price)
        : WithRoles(position, nccu::engine::math::Rect{position.x, position.y, 16.0f, 16.0f}, std::move(name)),
          price_(price) {}

    /**
     * @brief 互動即拾取入背包（與 OnPickup 同走 Collect）。
     * @param initiator 互動發起者（玩家）。
     */
    void Interact(Player* initiator) override { Collect(initiator); }
    /**
     * @brief 拾取入背包（與 Interact 同走 Collect）。
     * @param player 拾取者。
     */
    void OnPickup(Player* player) override { Collect(player); }

    /**
     * @brief 多型效果：於「使用」時施放（而非拾取時）。
     * @param player 使用者。
     *
     * 拾取只把道具計入背包；效果延後到玩家從背包使用時才套用，由 GameController
     * 導向 ApplyConsumableEffect（與此處各葉類別 Consume() 的業力增減與訊息一致）。
     * 具體子類別僅透過 EventBus 發事件，絕不直接呼叫 raylib。
     */
    virtual void Consume(Player* player) = 0;

    /** @brief 取得售價。@return 攤販售價。 */
    [[nodiscard]] int GetPrice() const noexcept { return price_; }

protected:
    int price_;   ///< 攤販售價

private:
    /**
     * @brief 拾取入背包，供兩條互動路徑共用。
     * @param player 拾取者。
     *
     * 以 isActive_ 保證冪等：已被拾取的世界物件即惰性，避免重複碰撞重複加入。
     * 拾取後標記為非存活，由幀末清除階段移除。
     */
    void Collect(Player* player) {
        if (!player || !isActive_) return;
        player->AddConsumable(GetName());
        isActive_ = false;
    }
};

#endif // CONSUMABLE_ITEM_H_
