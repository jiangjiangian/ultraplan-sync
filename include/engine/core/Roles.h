#ifndef ENTITY_ROLES_H_
#define ENTITY_ROLES_H_
#include <concepts>

// Role-interface split (ISP) + a CRTP static-dispatch mixin.
//
// Before this header GameObject was a "fat interface": every entity had
// to implement Update / Render / Interact even when the body was an empty
// no-op (ConsumableItem::Update/Render, Player::Interact). Splitting those
// three responsibilities into three INDEPENDENT role interfaces lets a
// class advertise exactly the roles it actually plays — the Interface
// Segregation Principle. The heterogeneous scene container still holds
// GameObject* and dispatches through GameObject's capability accessors
// (AsUpdatable / AsDrawable / AsInteractable), so runtime polymorphism
// stays where it belongs (one vector, many concrete types).
//
// The accessors themselves are bound by STATIC polymorphism: WithRoles is
// a CRTP mixin that compile-time-detects (std::derived_from + if
// constexpr) which role interfaces the most-derived type inherits and
// returns a static_cast'd pointer — no dynamic_cast, no per-call type
// check. See docs/architecture-roles.md for the rationale.

namespace nccu::engine::render { class IRenderer; }
class Player;  // role hooks take a Player* initiator; full type not needed here

// ── The three role interfaces (independent; none derives GameObject) ──
struct IUpdatable {
    virtual ~IUpdatable() = default;
    virtual void Update(float deltaTime) = 0;
};

struct IDrawable {
    virtual ~IDrawable() = default;
    virtual void Render(nccu::engine::render::IRenderer& renderer) const = 0;
};

struct IInteractable {
    virtual ~IInteractable() = default;
    virtual void Interact(Player* initiator) = 0;
};

// Assignment-#6 combat scaffolding. A fourth INDEPENDENT role (same ISP
// shape as the three above — no data members, behaviour contract only):
// an entity that has hit-points and can be damaged / killed. The #6
// Vampire-Survivors survival game needs a mortal player + mortal enemies;
// modelling "can take damage" as its own role keeps it out of GameObject's
// base (an item / decoration is NOT mortal) exactly as ISP wants. Today
// only Player plays it; enemies added in #6 will too. Default-implemented
// here (hp lives in the concrete class) so a leaf opts in by inheriting it
// and overriding — but the common hp_/TakeDamage/IsDead shape is provided
// as a convenience base a class can also just inherit verbatim.
struct IMortal {
    virtual ~IMortal() = default;
    // Apply `amount` damage (clamped at 0). noexcept — never throws in the
    // combat hot loop. A negative amount is ignored (use a heal API for
    // recovery; damage only ever lowers hp).
    virtual void TakeDamage(int amount) noexcept = 0;
    // Out of hit-points?
    [[nodiscard]] virtual bool IsDead() const noexcept = 0;
    // Current hit-points (>= 0). For HUD / tests / #6 combat math.
    [[nodiscard]] virtual int  Hp() const noexcept = 0;
};

// ── CRTP dispatch mixin ──────────────────────────────────────────────
// WithRoles<Derived, Base> injects itself between an existing
// GameObject-derived state base (Item / Character / ConsumableItem /
// TransparentUmbrella …) and the most-derived `Derived`. It STATICALLY
// implements GameObject's role-capability accessors by compile-time
// detecting which role interfaces `Derived` inherits. Apply it at the
// level where the role SET is fixed: on an intermediate when every leaf
// under it shares the same roles (ConsumableItem, TransparentUmbrella,
// NPC), or on the leaf itself when leaves differ.
template <class Derived, class Base>
class WithRoles : public Base {
public:
    using Base::Base;   // inherit Base's constructors

    IUpdatable* AsUpdatable() noexcept override {
        if constexpr (std::derived_from<Derived, IUpdatable>)
            return static_cast<Derived*>(this);
        else
            return nullptr;
    }
    const IDrawable* AsDrawable() const noexcept override {
        if constexpr (std::derived_from<Derived, IDrawable>)
            return static_cast<const Derived*>(this);
        else
            return nullptr;
    }
    IInteractable* AsInteractable() noexcept override {
        if constexpr (std::derived_from<Derived, IInteractable>)
            return static_cast<Derived*>(this);
        else
            return nullptr;
    }
    IMortal* AsMortal() noexcept override {
        if constexpr (std::derived_from<Derived, IMortal>)
            return static_cast<Derived*>(this);
        else
            return nullptr;
    }
};

// ── Generic role-dispatch helper (template showcase) ─────────────────
// Iterate a container of std::unique_ptr<GameObject> (or any range whose
// elements deref to a GameObject&), invoking fn(role) for every active
// object whose As<Role>() is non-null. The Role -> accessor mapping is
// resolved at compile time with if constexpr, so adding a fourth role
// later is one more branch here, not a change at every call site.
//
// Note: only the mutable roles (IUpdatable / IInteractable) go through
// this helper. The const Render path (IDrawable) is dispatched directly
// in the View because its painter's-order pass needs the GameObject for
// the depth key anyway — keeping it direct is clearer than a const
// overload here (the helper stays a single, honest utility).
template <class Role, class Container, class F>
void ForEachRole(Container& objects, F&& fn) {
    for (auto& obj : objects) {
        if (!obj || !obj->IsActive()) continue;
        if constexpr (std::same_as<Role, IUpdatable>) {
            if (IUpdatable* r = obj->AsUpdatable()) fn(*r);
        } else if constexpr (std::same_as<Role, IInteractable>) {
            if (IInteractable* r = obj->AsInteractable()) fn(*r);
        } else if constexpr (std::same_as<Role, IMortal>) {
            // Assignment-#6 combat: visit every mortal entity (the player
            // + future enemies) so damage / death sweeps are one call.
            if (IMortal* r = obj->AsMortal()) fn(*r);
        } else {
            static_assert(std::same_as<Role, IUpdatable>,
                          "ForEachRole supports the mutable roles "
                          "(IUpdatable / IInteractable / IMortal); render "
                          "through GameObject::AsDrawable() directly.");
        }
    }
}

#endif // ENTITY_ROLES_H_
