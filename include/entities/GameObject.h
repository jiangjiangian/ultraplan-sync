#ifndef GAME_OBJECT_H_
#define GAME_OBJECT_H_
#include "gfx/Vec2.h"
#include "gfx/Rect.h"
#include <string>
#include <string_view>
#include <vector>

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

    // Talk target? Default: nullptr (items, player, decoration). NPCs
    // override to return their line vector. Virtual-not-dynamic_cast,
    // same rationale as BlocksMovement(). Pointer (not ref) so "no
    // dialog" is representable without a sentinel vector.
    [[nodiscard]] virtual const std::vector<std::string>*
        DialogLines() const noexcept { return nullptr; }

    // Talk identity for runtime dialog lookup. Default: empty (items, the
    // player, Vendor, decoration, ambient students — they fall back to
    // Interact()). Archetype NPCs override to return their npcId so
    // GameController builds the per-(npcId, SemesterState) opener.
    // Virtual-not-dynamic_cast, same rationale as BlocksMovement().
    [[nodiscard]] virtual std::string_view NpcId() const noexcept { return {}; }

    // Shop counter? Default: false (everything but Vendor). A Vendor has
    // an empty NpcId() (it is not a dialog-content NPC) so without this
    // the E-interact path would route it to NPC::Interact line-cycling
    // and TryBuy would never run (BUGLEDGER I5). GameController checks
    // this to drive the purchase choice UI. Virtual-not-dynamic_cast,
    // same closed-under-inheritance rationale as BlocksMovement().
    [[nodiscard]] virtual bool IsVendor() const noexcept { return false; }

    // Quest-giver marker. Default: false (items, the player, Vendor,
    // decoration, non-quest NPCs). Archetype NPCs whose spawn flagged
    // them as quest-givers override to return true; the View uses this
    // to paint a "!" overlay above their sprite so the player can spot
    // the dialog hook at a glance (H4). Virtual-not-dynamic_cast, same
    // closed-under-inheritance rationale as BlocksMovement().
    [[nodiscard]] virtual bool IsQuestGiver() const noexcept { return false; }

protected:
    nccu::gfx::Vec2 position_;
    nccu::gfx::Rect hitBox_;
    bool isActive_;
    int collisionLayer_;
};

#endif // GAME_OBJECT_H_
