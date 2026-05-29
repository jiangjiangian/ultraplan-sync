#ifndef GAME_OBJECT_H_
#define GAME_OBJECT_H_
#include "engine/math/Vec2.h"
#include "engine/math/Rect.h"
#include "engine/core/Roles.h"   // IUpdatable / IDrawable / IInteractable + WithRoles
#include <string>
#include <string_view>
#include <vector>

namespace nccu::engine::render { class IRenderer; }
class Player; // forward decl — avoids circular include from the role hooks

class GameObject {
public:
    GameObject(nccu::engine::math::Vec2 position, nccu::engine::math::Rect hitBox)
        : position_(position), hitBox_(hitBox), isActive_(true), collisionLayer_(0) {}

    virtual ~GameObject() = default;

    // Role-capability accessors (ISP). Update / Render / Interact are no
    // longer fat pure-virtuals every entity must implement; instead a
    // concrete entity inherits exactly the role interfaces it plays
    // (IUpdatable / IDrawable / IInteractable) and the CRTP WithRoles
    // mixin STATICALLY implements these to hand back a typed pointer (or
    // null) — no dynamic_cast. The scene container dispatches through
    // them: `if (auto* u = o.AsUpdatable()) u->Update(dt);`. Default null
    // = "does not play this role" (skipped by the loop). See Roles.h.
    virtual IUpdatable*      AsUpdatable()      noexcept { return nullptr; }
    virtual const IDrawable* AsDrawable() const noexcept { return nullptr; }
    virtual IInteractable*   AsInteractable()   noexcept { return nullptr; }
    // Assignment-#6 combat scaffolding: a fourth role accessor, same shape
    // as the three above. Default null = "not mortal" (items, decoration);
    // Player (and #6 enemies) override via WithRoles to hand back an
    // IMortal*. The scene container can then ForEachRole<IMortal>(...) to
    // run a damage / death pass over exactly the mortal entities.
    virtual IMortal*         AsMortal()          noexcept { return nullptr; }

    [[nodiscard]] bool CheckCollision(nccu::engine::math::Rect other) const noexcept {
        return hitBox_.Intersects(other);
    }

    [[nodiscard]] bool IsActive() const noexcept { return isActive_; }
    void Deactivate() noexcept { isActive_ = false; }
    [[nodiscard]] nccu::engine::math::Vec2 GetPosition() const noexcept { return position_; }

    // Collision-layer tag (review MINOR: collisionLayer_ was set in the
    // ctor but had no reader — dead state). 0 = the default layer (player,
    // NPCs, items today). Exposed for the Assignment-#6 survival game,
    // where layering (player / enemy / projectile / pickup) lets the
    // CollisionSystem decide which pairs actually interact. SetCollision
    // Layer lets a future spawner tag an entity without a new ctor arg.
    [[nodiscard]] int  GetCollisionLayer() const noexcept {
        return collisionLayer_;
    }
    void SetCollisionLayer(int layer) noexcept { collisionLayer_ = layer; }

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
    nccu::engine::math::Vec2 position_;
    nccu::engine::math::Rect hitBox_;
    bool isActive_;
    int collisionLayer_;
};

#endif // GAME_OBJECT_H_
