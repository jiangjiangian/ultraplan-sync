#ifndef CONSUMABLE_ITEM_H_
#define CONSUMABLE_ITEM_H_
#include "entities/Item.h"
#include "entities/Player.h"

// ConsumableItem: abstract Item that the player collects into the bag.
// Mirrors TransparentUmbrella's role as a middle layer between Item and the
// concrete subclasses; the polymorphic verb here is Consume() instead of
// beClaimed().
//
// ISP roles: IInteractable ONLY. Its old Update/Render bodies were empty
// no-ops (nothing to tick, the View owns drink rendering), so dropping
// those roles means the update/render loops simply skip it — identical
// behaviour, fewer empty overrides. Every leaf (EnergyDrink / HotPack /
// WaterproofSpray) shares this exact role set, so WithRoles is keyed on
// this intermediate (Derived = ConsumableItem); static_cast<ConsumableItem*>
// is valid for each leaf since they all IS-A ConsumableItem.
class ConsumableItem : public WithRoles<ConsumableItem, Item>, public IInteractable {
public:
    ConsumableItem(nccu::gfx::Vec2 position, std::string name, int price)
        : WithRoles(position, nccu::gfx::Rect{position.x, position.y, 16.0f, 16.0f}, std::move(name)),
          price_(price) {}

    // Item 2(a) — hold-and-use bag: picking a consumable up off the world
    // now ADDS it to the player's count inventory (under its itemId =
    // GetName()) instead of firing Consume() on the spot. The EFFECT is no
    // longer applied here; it is applied later when the player USES the
    // item from the open bag (GameController routes that to
    // ApplyConsumableEffect, which shares each Consume() body's exact
    // karma delta + ShowMessage). The isActive_ idempotency guard is kept:
    // a pickup deactivates the world object exactly once (end-of-frame
    // sweep removes it), so a re-collision can't double-add. Consume()
    // itself is UNCHANGED (still the per-leaf effect) — the bookworm
    // rescue's quest consumption and the entity unit tests call it
    // directly; only the pickup wiring moved off it.
    void Interact(Player* initiator) override { Collect(initiator); }
    void OnPickup(Player* player) override { Collect(player); }

    // The polymorphic effect — applied on USE (from the bag), not on
    // pickup. Concrete subclasses emit events via EventBus, never raylib.
    virtual void Consume(Player* player) = 0;

    [[nodiscard]] int GetPrice() const noexcept { return price_; }

protected:
    int price_;

private:
    // Pickup-to-bag, shared by both interact paths. Idempotent via
    // isActive_ (a consumed/collected world object is inert).
    void Collect(Player* player) {
        if (!player || !isActive_) return;
        player->AddConsumable(GetName());
        isActive_ = false;
    }
};

#endif // CONSUMABLE_ITEM_H_
