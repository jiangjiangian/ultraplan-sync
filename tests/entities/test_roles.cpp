#include "doctest/doctest.h"
#include "engine/core/GameObject.h"
#include "engine/core/Roles.h"
#include "game/entities/Player.h"
#include "game/entities/NPC.h"
#include "game/entities/HotPack.h"
#include "game/entities/EnergyDrink.h"
#include "game/entities/TrueUmbrella.h"
#include "game/entities/CashPickup.h"
#include "game/entities/QuestFlagPickup.h"
#include "game/vendor/Vendor.h"
#include "game/vendor/VendorConfig.h"
#include "engine/events/EventBus.h"

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
    Player p{nccu::engine::math::Vec2{0, 0}};
    GameObject& g = p;
    CHECK(g.AsUpdatable()    != nullptr);   // Update is real
    CHECK(g.AsDrawable()     != nullptr);   // Render is real
    CHECK(g.AsInteractable() == nullptr);   // old Interact body was empty
}

TEST_CASE("NPC plays all three roles") {
    NPC n{nccu::engine::math::Vec2{0, 0}, std::vector<std::string>{"hi"}};
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
    Vendor v{nccu::engine::math::Vec2{0, 0}, cfg};
    GameObject& g = v;
    CHECK(g.AsUpdatable()    != nullptr);
    CHECK(g.AsDrawable()     != nullptr);
    CHECK(g.AsInteractable() != nullptr);
    CHECK(g.IsVendor());                     // classification query intact
}

TEST_CASE("ConsumableItem plays Interact ONLY (Update + Render no-ops dropped)") {
    HotPack pack{nccu::engine::math::Vec2{0, 0}};
    GameObject& g = pack;
    CHECK(g.AsUpdatable()    == nullptr);    // old Update body was empty
    CHECK(g.AsDrawable()     == nullptr);    // old Render body was empty
    CHECK(g.AsInteractable() != nullptr);    // Interact -> Consume is real

    EnergyDrink drink{nccu::engine::math::Vec2{0, 0}};
    GameObject& gd = drink;
    CHECK(gd.AsUpdatable()    == nullptr);
    CHECK(gd.AsDrawable()     == nullptr);
    CHECK(gd.AsInteractable() != nullptr);
}

TEST_CASE("Umbrella plays Draw + Interact but NOT Update (no-op dropped)") {
    TrueUmbrella u{nccu::engine::math::Vec2{0, 0}};
    GameObject& g = u;
    CHECK(g.AsUpdatable()    == nullptr);    // old Update body was empty
    CHECK(g.AsDrawable()     != nullptr);    // per-style glyph render is real
    CHECK(g.AsInteractable() != nullptr);    // quest-gated claim is real
}

TEST_CASE("Cash / quest pickups play Draw + Interact but NOT Update") {
    CashPickup cash{nccu::engine::math::Vec2{0, 0}, 50};
    GameObject& gc = cash;
    CHECK(gc.AsUpdatable()    == nullptr);
    CHECK(gc.AsDrawable()     != nullptr);
    CHECK(gc.AsInteractable() != nullptr);

    QuestFlagPickup form{nccu::engine::math::Vec2{0, 0}, "Flag_X"};
    GameObject& gf = form;
    CHECK(gf.AsUpdatable()    == nullptr);
    CHECK(gf.AsDrawable()     != nullptr);
    CHECK(gf.AsInteractable() != nullptr);
}

TEST_CASE("A bare GameObject subclass plays no roles") {
    struct Bare final : GameObject {
        Bare() : GameObject(nccu::engine::math::Vec2{0, 0}, nccu::engine::math::Rect{0, 0, 1, 1}) {}
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
    Player p{nccu::engine::math::Vec2{0, 0}};
    QuestFlagPickup form{nccu::engine::math::Vec2{0, 0}, "Flag_RoleDispatch"};
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
    objs.push_back(std::make_unique<Player>(nccu::engine::math::Vec2{0, 0}));
    objs.push_back(std::make_unique<NPC>(nccu::engine::math::Vec2{0, 0},
                                         std::vector<std::string>{"x"}));
    objs.push_back(std::make_unique<TrueUmbrella>(nccu::engine::math::Vec2{0, 0}));
    objs.push_back(std::make_unique<HotPack>(nccu::engine::math::Vec2{0, 0}));
    objs.push_back(std::make_unique<CashPickup>(nccu::engine::math::Vec2{0, 0}, 10));

    int visited = 0;
    ForEachRole<IUpdatable>(objs, [&](IUpdatable&) { ++visited; });
    CHECK(visited == 2);                     // Player + NPC only

    // An inactive object is skipped (mark-then-sweep semantics preserved).
    objs.front()->Deactivate();
    visited = 0;
    ForEachRole<IUpdatable>(objs, [&](IUpdatable&) { ++visited; });
    CHECK(visited == 1);                     // just the NPC now
}

// ── IMortal (Assignment-#6 combat scaffolding) ──────────────────────
TEST_CASE("Player plays the IMortal role; NPC / items do not") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    GameObject& gp = p;
    CHECK(gp.AsMortal() != nullptr);         // the player has hit-points

    NPC n{nccu::engine::math::Vec2{0, 0}, std::vector<std::string>{"hi"}};
    GameObject& gn = n;
    CHECK(gn.AsMortal() == nullptr);         // an NPC is not mortal (today)

    TrueUmbrella u{nccu::engine::math::Vec2{0, 0}};
    GameObject& gu = u;
    CHECK(gu.AsMortal() == nullptr);         // an item is not mortal
}

TEST_CASE("IMortal: TakeDamage lowers hp, clamps at 0, IsDead flips") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    CHECK(p.Hp() == Player::kMaxHp);
    CHECK_FALSE(p.IsDead());
    p.TakeDamage(30);
    CHECK(p.Hp() == Player::kMaxHp - 30);
    CHECK_FALSE(p.IsDead());
    p.TakeDamage(-5);                        // non-positive ignored
    CHECK(p.Hp() == Player::kMaxHp - 30);
    p.TakeDamage(1000);                      // over-kill clamps at 0
    CHECK(p.Hp() == 0);
    CHECK(p.IsDead());
}

TEST_CASE("ForEachRole<IMortal> visits only mortal entities, dispatches damage") {
    std::vector<std::unique_ptr<GameObject>> objs;
    objs.push_back(std::make_unique<Player>(nccu::engine::math::Vec2{0, 0}));
    objs.push_back(std::make_unique<NPC>(nccu::engine::math::Vec2{0, 0},
                                         std::vector<std::string>{"x"}));
    objs.push_back(std::make_unique<TrueUmbrella>(nccu::engine::math::Vec2{0, 0}));

    int visited = 0;
    ForEachRole<IMortal>(objs, [&](IMortal& m) { ++visited; m.TakeDamage(10); });
    CHECK(visited == 1);                     // only the Player is mortal
    // The damage really landed through the dispatched IMortal&.
    CHECK(objs.front()->AsMortal()->Hp() == Player::kMaxHp - 10);
}

// ── GetCollisionLayer (was dead state; review MINOR) ────────────────
TEST_CASE("GameObject collision layer: default 0, settable") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    GameObject& g = p;
    CHECK(g.GetCollisionLayer() == 0);       // default layer
    g.SetCollisionLayer(3);
    CHECK(g.GetCollisionLayer() == 3);
}
