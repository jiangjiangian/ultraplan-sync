/**
 * @file test_quest_pickup.cpp
 * @brief 驗證 QuestFlagPickup（任務旗標拾取物）撿取後會在玩家身上設旗標並自我停用。
 */
#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/entities/QuestFlagPickup.h"
#include "game/entities/Player.h"
#include "engine/math/Vec2.h"

// 撿取後玩家獲得對應旗標，物件本身停用。
TEST_CASE("QuestFlagPickup sets its flag on the player and deactivates") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    QuestFlagPickup item(nccu::engine::math::Vec2{10, 10}, nccu::kFlagFoundForm);
    CHECK(item.IsActive());
    CHECK_FALSE(p.HasFlag(nccu::kFlagFoundForm));
    item.Interact(&p);
    CHECK(p.HasFlag(nccu::kFlagFoundForm));
    CHECK_FALSE(item.IsActive());
}

// 互動對象為 null 時是安全的空操作（不崩潰、狀態不變）。
TEST_CASE("QuestFlagPickup with a null initiator is a safe no-op") {
    QuestFlagPickup item(nccu::engine::math::Vec2{0, 0}, "Flag_X");
    item.Interact(nullptr);
    CHECK(item.IsActive());   // 維持原狀，不崩潰
}
