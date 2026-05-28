#include "doctest/doctest.h"
#include "quest/Flags.h"
#include "entities/QuestFlagPickup.h"
#include "entities/Player.h"
#include "gfx/Vec2.h"

TEST_CASE("QuestFlagPickup sets its flag on the player and deactivates") {
    Player p{nccu::gfx::Vec2{0, 0}};
    QuestFlagPickup item(nccu::gfx::Vec2{10, 10}, nccu::kFlagFoundForm);
    CHECK(item.IsActive());
    CHECK_FALSE(p.HasFlag(nccu::kFlagFoundForm));
    item.Interact(&p);
    CHECK(p.HasFlag(nccu::kFlagFoundForm));
    CHECK_FALSE(item.IsActive());
}

TEST_CASE("QuestFlagPickup with a null initiator is a safe no-op") {
    QuestFlagPickup item(nccu::gfx::Vec2{0, 0}, "Flag_X");
    item.Interact(nullptr);
    CHECK(item.IsActive());   // unchanged; no crash
}
