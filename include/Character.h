#pragma once
#include "GameObject.h"
#include <cmath>

class Character : public GameObject {
public:
    // speed_ is in pixels per second (frame-rate independent)
    Character(Vector2 position, Rectangle hitBox, float speed)
        : GameObject(position, hitBox), speed_(speed),
          direction_({0.0f, 0.0f}), currentFrame_(0) {}

    // Normalises dir so diagonal isn't √2 faster than cardinal,
    // then advances by speed_ × deltaTime (seconds).
    void Move(Vector2 dir, float deltaTime) {
        const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.0001f) {
            dir.x /= len;
            dir.y /= len;
        }
        position_.x += dir.x * speed_ * deltaTime;
        position_.y += dir.y * speed_ * deltaTime;
        hitBox_.x = position_.x;
        hitBox_.y = position_.y;
        direction_ = dir;
    }

protected:
    float speed_;
    Vector2 direction_;
    int currentFrame_;
};
