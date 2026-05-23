#include "doctest/doctest.h"
#include "entities/GameObject.h"
#include "entities/Roles.h"
#include "entities/Player.h"
#include "entities/NPC.h"
#include "entities/HotPack.h"
#include "entities/EnergyDrink.h"
#include "entities/TrueUmbrella.h"
#include "entities/CashPickup.h"
#include "entities/QuestFlagPickup.h"
#include "vendor/Vendor.h"
#include "vendor/VendorConfig.h"
#include "controller/EventBus.h"

#include <memory>
#include <vector>

// ISP role split + CRTP static dispatch (Roles.h / WithRoles).
//
// Every check views the entity through a GameObject& — i.e. exactly how
// the scene container sees it — so these prove the *static* As*() accessors
// resolve correctly through the runtime-polymorphic base. A role the class
// dropped (an empty no-op pre-refactor) must report null; a role it kept
// must hand back a usable pointer. No dynamic_cast is involved anywhere.

TEST_CASE("Player plays Update + Draw but NOT Interact (no-op dropped)") {
    Player p{nccu::gfx::Vec2{0, 0}};
    GameObject& g = p;
    CHECK(g.AsUpdatable()    != nullptr);   // Update is real
    CHECK(g.AsDrawable()     != nullptr);   // Render is real
    CHECK(g.AsInteractable() == nullptr);   // old Interact body was empty
}

TEST_CASE("NPC plays all three roles") {
    NPC n{nccu::gfx::Vec2{0, 0}, std::vector<std::string>{"hi"}};
    GameObject& g = n;
    CHECK(g.AsUpdatable()    != nullptr);
    CHECK(g.AsDrawable()     != nullptr);
    CHECK(g.AsInteractable() != nullptr);
}

TEST_CASE("Vendor (NPC subclass) inherits NPC's full role set via WithRoles") {
    // Proves WithRoles keyed on the NPC intermediate dispatches correctly
    // for a more-derived leaf: a Vendor IS-A NPC, so static_cast<NPC*> in
    // the accessor is valid and all three roles resolve.
    VendorConfig cfg;
    cfg.greeting = "歡迎光臨";
    Vendor v{nccu::gfx::Vec2{0, 0}, cfg};
    GameObject& g = v;
    CHECK(g.AsUpdatable()    != nullptr);
    CHECK(g.AsDrawable()     != nullptr);
    CHECK(g.AsInteractable() != nullptr);
    CHECK(g.IsVendor());                     // classification query intact
}

TEST_CASE("ConsumableItem plays Interact ONLY (Update + Render no-ops dropped)") {
    HotPack pack{nccu::gfx::Vec2{0, 0}};
    GameObject& g = pack;
    CHECK(g.AsUpdatable()    == nullptr);    // old Update body was empty
    CHECK(g.AsDrawable()     == nullptr);    // old Render body was empty
    CHECK(g.AsInteractable() != nullptr);    // Interact -> Consume is real

    EnergyDrink drink{nccu::gfx::Vec2{0, 0}};
    GameObject& gd = drink;
    CHECK(gd.AsUpdatable()    == nullptr);
    CHECK(gd.AsDrawable()     == nullptr);
    CHECK(gd.AsInteractable() != nullptr);
}

TEST_CASE("Umbrella plays Draw + Interact but NOT Update (no-op dropped)") {
    TrueUmbrella u{nccu::gfx::Vec2{0, 0}};
    GameObject& g = u;
    CHECK(g.AsUpdatable()    == nullptr);    // old Update body was empty
    CHECK(g.AsDrawable()     != nullptr);    // per-style glyph render is real
    CHECK(g.AsInteractable() != nullptr);    // quest-gated claim is real
}

TEST_CASE("Cash / quest pickups play Draw + Interact but NOT Update") {
    CashPickup cash{nccu::gfx::Vec2{0, 0}, 50};
    GameObject& gc = cash;
    CHECK(gc.AsUpdatable()    == nullptr);
    CHECK(gc.AsDrawable()     != nullptr);
    CHECK(gc.AsInteractable() != nullptr);

    QuestFlagPickup form{nccu::gfx::Vec2{0, 0}, "Flag_X"};
    GameObject& gf = form;
    CHECK(gf.AsUpdatable()    == nullptr);
    CHECK(gf.AsDrawable()     != nullptr);
    CHECK(gf.AsInteractable() != nullptr);
}

TEST_CASE("A bare GameObject subclass plays no roles") {
    struct Bare final : GameObject {
        Bare() : GameObject(nccu::gfx::Vec2{0, 0}, nccu::gfx::Rect{0, 0, 1, 1}) {}
    };
    Bare b;
    GameObject& g = b;
    CHECK(g.AsUpdatable()    == nullptr);
    CHECK(g.AsDrawable()     == nullptr);
    CHECK(g.AsInteractable() == nullptr);
}

TEST_CASE("The static accessor returns a pointer that really dispatches") {
    // Not just non-null: the returned IInteractable* must invoke the
    // concrete override. A QuestFlagPickup sets its flag on Interact.
    Player p{nccu::gfx::Vec2{0, 0}};
    QuestFlagPickup form{nccu::gfx::Vec2{0, 0}, "Flag_RoleDispatch"};
    GameObject& g = form;
    REQUIRE(g.AsInteractable() != nullptr);
    CHECK_FALSE(p.HasFlag("Flag_RoleDispatch"));
    g.AsInteractable()->Interact(&p);
    CHECK(p.HasFlag("Flag_RoleDispatch"));
    CHECK_FALSE(form.IsActive());            // pickup deactivated as before
}

TEST_CASE("ForEachRole<IUpdatable> visits only the objects that tick") {
    // Mixed container, exactly the scene-container shape. Only Player and
    // NPC play IUpdatable; the umbrella / pickup / consumable must be
    // skipped (their old Update was an empty no-op).
    std::vector<std::unique_ptr<GameObject>> objs;
    objs.push_back(std::make_unique<Player>(nccu::gfx::Vec2{0, 0}));
    objs.push_back(std::make_unique<NPC>(nccu::gfx::Vec2{0, 0},
                                         std::vector<std::string>{"x"}));
    objs.push_back(std::make_unique<TrueUmbrella>(nccu::gfx::Vec2{0, 0}));
    objs.push_back(std::make_unique<HotPack>(nccu::gfx::Vec2{0, 0}));
    objs.push_back(std::make_unique<CashPickup>(nccu::gfx::Vec2{0, 0}, 10));

    int visited = 0;
    ForEachRole<IUpdatable>(objs, [&](IUpdatable&) { ++visited; });
    CHECK(visited == 2);                     // Player + NPC only

    // An inactive object is skipped (mark-then-sweep semantics preserved).
    objs.front()->Deactivate();
    visited = 0;
    ForEachRole<IUpdatable>(objs, [&](IUpdatable&) { ++visited; });
    CHECK(visited == 1);                     // just the NPC now
}
