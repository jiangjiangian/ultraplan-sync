#ifndef CONSUMABLE_ITEM_H_
#define CONSUMABLE_ITEM_H_
#include "entities/Item.h"

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
    void Render(nccu::gfx::IRenderer&) const override {} // nothing to draw; rendering owned by the View layer
    void Interact(Player* initiator) override { Consume(initiator); }
    void OnPickup(Player* player) override { Consume(player); }

    virtual void Consume(Player* player) = 0;

    [[nodiscard]] int GetPrice() const noexcept { return price_; }

protected:
    int price_;
};

#endif // CONSUMABLE_ITEM_H_
