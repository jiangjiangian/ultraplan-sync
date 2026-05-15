#ifndef GAME_OBJECT_H_
#define GAME_OBJECT_H_
#include "gfx/Vec2.h"
#include "gfx/Rect.h"

namespace nccu::gfx { class IRenderer; }
class Player; // forward decl — avoids circular include from Interact()

class GameObject {
public:
    GameObject(nccu::gfx::Vec2 position, nccu::gfx::Rect hitBox)
        : position_(position), hitBox_(hitBox), isActive_(true), collisionLayer_(0) {}

    virtual ~GameObject() = default;

    virtual void Update(float deltaTime) = 0;
    virtual void Render(nccu::gfx::IRenderer& renderer) const = 0;
    virtual void Interact(Player* initiator) = 0;

    [[nodiscard]] bool CheckCollision(nccu::gfx::Rect other) const noexcept {
        return hitBox_.Intersects(other);
    }

    [[nodiscard]] bool IsActive() const noexcept { return isActive_; }
    void Deactivate() noexcept { isActive_ = false; }
    [[nodiscard]] nccu::gfx::Vec2 GetPosition() const noexcept { return position_; }

    // Replaces dynamic_cast<NPC*> in the collision loop. Default: false
    // (items, the player, decoration). Movement blockers — NPCs, future
    // wall objects — override to return true. Virtual dispatch + bool is
    // closed under inheritance; dynamic_cast is not.
    [[nodiscard]] virtual bool BlocksMovement() const noexcept { return false; }

protected:
    nccu::gfx::Vec2 position_;
    nccu::gfx::Rect hitBox_;
    bool isActive_;
    int collisionLayer_;
};

#endif // GAME_OBJECT_H_
