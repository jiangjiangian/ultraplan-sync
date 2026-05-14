#ifndef CONSUMABLE_ITEM_H_
#define CONSUMABLE_ITEM_H_
#include "Item.h"

// ConsumableItem: abstract Item that disappears after being used.
// Mirrors TransparentUmbrella's role as a middle layer between Item and the
// concrete subclasses; the polymorphic verb here is Consume() instead of
// beClaimed().
class ConsumableItem : public Item {
public:
    ConsumableItem(nccu::gfx::Vec2 position, std::string name, int price)
        : Item(position, nccu::gfx::Rect{position.x, position.y, 16.0f, 16.0f}, std::move(name)),
          price_(price) {}

    // Pure data — concrete subclasses emit events via EventBus, never raylib.
    void Update(float /*deltaTime*/) override {}
    void Draw() const override {} // no raylib call; rendering owned by gfx layer
    void Interact(Player* initiator) override { Consume(initiator); }
    void OnPickup(Player* player) override { Consume(player); }

    virtual void Consume(Player* player) = 0;

    int GetPrice() const { return price_; }

protected:
    int price_;
};

#endif // CONSUMABLE_ITEM_H_
