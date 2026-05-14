#ifndef TRANSPARENT_UMBRELLA_H_
#define TRANSPARENT_UMBRELLA_H_
#include "Item.h"
#include "gfx/Color.h"

class TransparentUmbrella : public Item {
public:
    TransparentUmbrella(nccu::gfx::Vec2 position, std::string name, nccu::gfx::Color tint)
        : Item(position, nccu::gfx::Rect{position.x, position.y, 20.0f, 20.0f}, std::move(name)),
          umbrellaTint_(tint) {}

    void Update(float /*deltaTime*/) override {}
    void Draw() const override; // emits event via EventBus, no raylib call
    void Interact(Player* initiator) override { beClaimed(initiator); }
    void OnPickup(Player* player) override { beClaimed(player); }

    virtual void beClaimed(Player* player) = 0;

protected:
    nccu::gfx::Color umbrellaTint_;
};

#endif // TRANSPARENT_UMBRELLA_H_
