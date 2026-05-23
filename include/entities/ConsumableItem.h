#ifndef CONSUMABLE_ITEM_H_
#define CONSUMABLE_ITEM_H_
#include "entities/Item.h"

// ConsumableItem: abstract Item that disappears after being used.
// Mirrors TransparentUmbrella's role as a middle layer between Item and the
// concrete subclasses; the polymorphic verb here is Consume() instead of
// beClaimed().
//
// ISP roles: IInteractable ONLY. Its old Update/Render bodies were empty
// no-ops (nothing to tick, the View owns drink rendering), so dropping
// those roles means the update/render loops simply skip it — identical
// behaviour, fewer empty overrides. Every leaf (EnergyDrink / HotPack /
// WaterproofSpray) shares this exact role set, so WithRoles is keyed on
// this intermediate (Derived = ConsumableItem); static_cast<ConsumableItem*>
// is valid for each leaf since they all IS-A ConsumableItem.
class ConsumableItem : public WithRoles<ConsumableItem, Item>, public IInteractable {
public:
    ConsumableItem(nccu::gfx::Vec2 position, std::string name, int price)
        : WithRoles(position, nccu::gfx::Rect{position.x, position.y, 16.0f, 16.0f}, std::move(name)),
          price_(price) {}

    // Both pick-up paths funnel into Consume(). Pure data — concrete
    // subclasses emit events via EventBus, never raylib.
    void Interact(Player* initiator) override { Consume(initiator); }
    void OnPickup(Player* player) override { Consume(player); }

    virtual void Consume(Player* player) = 0;

    [[nodiscard]] int GetPrice() const noexcept { return price_; }

protected:
    int price_;
};

#endif // CONSUMABLE_ITEM_H_
