#pragma once
#include "raylib.h"

class Player; // forward decl — avoids circular include from Interact()

class GameObject {
public:
    GameObject(Vector2 position, Rectangle hitBox)
        : position_(position), hitBox_(hitBox), isActive_(true), collisionLayer_(0) {}

    virtual ~GameObject() = default;

    virtual void Update(float deltaTime) = 0;
    virtual void Draw() const = 0;
    virtual void Interact(Player* initiator) = 0;

    bool CheckCollision(Rectangle other) const {
        return CheckCollisionRecs(hitBox_, other);
    }

    bool IsActive() const { return isActive_; }
    void Deactivate() { isActive_ = false; }
    Vector2 GetPosition() const { return position_; }

protected:
    Vector2 position_;
    Rectangle hitBox_;
    bool isActive_;
    int collisionLayer_;
};
