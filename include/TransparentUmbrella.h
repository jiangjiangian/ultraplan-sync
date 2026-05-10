#pragma once
#include "Item.h"

class TransparentUmbrella : public Item {
public:
    TransparentUmbrella(Vector2 position, std::string name, Color tint)
        : Item(position, {position.x, position.y, 20.0f, 20.0f}, std::move(name)),
          umbrellaTint_(tint) {}

    void Update(float /*deltaTime*/) override {}
    void Draw() const override; // emits event via EventBus, no raylib call
    void Interact(Player* initiator) override { beClaimed(initiator); }
    void OnPickup(Player* player) override { beClaimed(player); }

    virtual void beClaimed(Player* player) = 0;

protected:
    Color umbrellaTint_;
};
