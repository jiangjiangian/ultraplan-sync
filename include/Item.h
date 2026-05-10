#pragma once
#include "GameObject.h"
#include <string>

class Item : public GameObject {
public:
    Item(Vector2 position, Rectangle hitBox, std::string name)
        : GameObject(position, hitBox), itemName_(std::move(name)), isPickable_(true) {}

    virtual void OnPickup(Player* player) = 0;

    const std::string& GetName() const { return itemName_; }
    bool IsPickable() const { return isPickable_; }

protected:
    std::string itemName_;
    bool isPickable_;
};
