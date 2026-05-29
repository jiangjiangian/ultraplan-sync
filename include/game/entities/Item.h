#ifndef ITEM_H_
#define ITEM_H_
#include "engine/core/GameObject.h"
#include <string>

class Item : public GameObject {
public:
    Item(nccu::engine::math::Vec2 position, nccu::engine::math::Rect hitBox, std::string name)
        : GameObject(position, hitBox), itemName_(std::move(name)), isPickable_(true) {}

    virtual void OnPickup(Player* player) = 0;

    [[nodiscard]] const std::string& GetName() const noexcept { return itemName_; }
    [[nodiscard]] bool IsPickable() const noexcept { return isPickable_; }

protected:
    std::string itemName_;
    bool isPickable_;
};

#endif // ITEM_H_
