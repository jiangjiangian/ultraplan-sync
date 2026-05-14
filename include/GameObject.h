#ifndef GAME_OBJECT_H_
#define GAME_OBJECT_H_
#include "gfx/Vec2.h"
#include "gfx/Rect.h"

class Player; // forward decl — avoids circular include from Interact()

class GameObject {
public:
    GameObject(nccu::gfx::Vec2 position, nccu::gfx::Rect hitBox)
        : position_(position), hitBox_(hitBox), isActive_(true), collisionLayer_(0) {}

    virtual ~GameObject() = default;

    virtual void Update(float deltaTime) = 0;
    virtual void Draw() const = 0;
    virtual void Interact(Player* initiator) = 0;

    bool CheckCollision(nccu::gfx::Rect other) const {
        return hitBox_.Intersects(other);
    }

    bool IsActive() const { return isActive_; }
    void Deactivate() { isActive_ = false; }
    nccu::gfx::Vec2 GetPosition() const { return position_; }

protected:
    nccu::gfx::Vec2 position_;
    nccu::gfx::Rect hitBox_;
    bool isActive_;
    int collisionLayer_;
};

#endif // GAME_OBJECT_H_
