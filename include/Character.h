#pragma once
#include "GameObject.h"
#include "gfx/Vec2.h"

class Character : public GameObject {
public:
    // speed_ is in pixels per second (frame-rate independent)
    Character(nccu::gfx::Vec2 position, nccu::gfx::Rect hitBox, float speed)
        : GameObject(position, hitBox), speed_(speed),
          direction_({0.0f, 0.0f}), currentFrame_(0) {}

    // Normalises dir so diagonal isn't sqrt(2) faster than cardinal,
    // then advances by speed_ × deltaTime (seconds).
    void Move(nccu::gfx::Vec2 dir, float deltaTime) {
        const nccu::gfx::Vec2 n = dir.Normalized();
        position_.x += n.x * speed_ * deltaTime;
        position_.y += n.y * speed_ * deltaTime;
        hitBox_.x = position_.x;
        hitBox_.y = position_.y;
        direction_ = n;
    }

protected:
    float speed_;
    nccu::gfx::Vec2 direction_;
    int currentFrame_;
};
