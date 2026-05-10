#pragma once
#include "GameObject.h"

class Character : public GameObject {
public:
    Character(Vector2 position, Rectangle hitBox, float speed)
        : GameObject(position, hitBox), speed_(speed),
          direction_({0.0f, 0.0f}), currentFrame_(0) {}

    void Move(Vector2 dir) {
        position_.x += dir.x * speed_;
        position_.y += dir.y * speed_;
        hitBox_.x = position_.x;
        hitBox_.y = position_.y;
        direction_ = dir;
    }

protected:
    float speed_;
    Vector2 direction_;
    int currentFrame_;
};
