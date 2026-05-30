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

/**
 * @file test_roles.cpp
 * @brief 驗證以介面分離（ISP）切出的角色與 CRTP 靜態分派（Roles.h / WithRoles）：
 *        各實體透過 GameObject& 查詢 As*() 角色存取器是否正確解析，包含 IMortal
 *        角色、ForEachRole 的遍歷，以及碰撞層位的存取。
 */

// 每個檢查都透過 GameObject& 觀察實體（正是場景容器看待它的方式），藉此證明
// 靜態的 As*() 存取器能透過執行期多型的基底正確解析。類別捨棄的角色（重構前是
// 空的空操作）必須回傳 null；保留的角色必須回傳可用的指標。全程不使用 dynamic_cast。

// Player 扮演 Update + Draw，但不扮演 Interact（空操作已捨棄）。
TEST_CASE("Player 扮演 Update + Draw，但不扮演 Interact（空操作已捨棄）") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    GameObject& g = p;
    CHECK(g.AsUpdatable()    != nullptr);   // Update 有實作
    CHECK(g.AsDrawable()     != nullptr);   // Render 有實作
    CHECK(g.AsInteractable() == nullptr);   // 舊的 Interact 內容為空
}

// NPC 扮演全部三種角色。
TEST_CASE("NPC 扮演全部三種角色") {
    NPC n{nccu::engine::math::Vec2{0, 0}, std::vector<std::string>{"hi"}};
    GameObject& g = n;
    CHECK(g.AsUpdatable()    != nullptr);
    CHECK(g.AsDrawable()     != nullptr);
    CHECK(g.AsInteractable() != nullptr);
}

// Vendor（NPC 子類）透過 WithRoles 繼承 NPC 的完整角色集。
TEST_CASE("Vendor（NPC 子類）透過 WithRoles 繼承 NPC 的完整角色集") {
    // 證明以 NPC 中介層為鍵的 WithRoles，對更衍生的葉節點也能正確分派：
    // Vendor IS-A NPC，因此存取器中的 static_cast<NPC*> 合法，三種角色皆能解析。
    VendorConfig cfg;
    cfg.greeting = "歡迎光臨";
    Vendor v{nccu::engine::math::Vec2{0, 0}, cfg};
    GameObject& g = v;
    CHECK(g.AsUpdatable()    != nullptr);
    CHECK(g.AsDrawable()     != nullptr);
    CHECK(g.AsInteractable() != nullptr);
    CHECK(g.IsVendor());                     // 分類查詢仍可用
}

// ConsumableItem 只扮演 Interact（Update + Render 空操作已捨棄）。
TEST_CASE("ConsumableItem 只扮演 Interact（Update + Render 空操作已捨棄）") {
    HotPack pack{nccu::engine::math::Vec2{0, 0}};
    GameObject& g = pack;
    CHECK(g.AsUpdatable()    == nullptr);    // 舊的 Update 內容為空
    CHECK(g.AsDrawable()     == nullptr);    // 舊的 Render 內容為空
    CHECK(g.AsInteractable() != nullptr);    // Interact -> Consume 有實作

    EnergyDrink drink{nccu::engine::math::Vec2{0, 0}};
    GameObject& gd = drink;
    CHECK(gd.AsUpdatable()    == nullptr);
    CHECK(gd.AsDrawable()     == nullptr);
    CHECK(gd.AsInteractable() != nullptr);
}

// 雨傘扮演 Draw + Interact，但不扮演 Update（空操作已捨棄）。
TEST_CASE("雨傘扮演 Draw + Interact，但不扮演 Update（空操作已捨棄）") {
    TrueUmbrella u{nccu::engine::math::Vec2{0, 0}};
    GameObject& g = u;
    CHECK(g.AsUpdatable()    == nullptr);    // 舊的 Update 內容為空
    CHECK(g.AsDrawable()     != nullptr);    // 各款式的圖形算繪有實作
    CHECK(g.AsInteractable() != nullptr);    // 受任務條件控制的取傘有實作
}

// 金錢／任務拾取物扮演 Draw + Interact，但不扮演 Update。
TEST_CASE("金錢／任務拾取物扮演 Draw + Interact，但不扮演 Update") {
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

// 純粹的 GameObject 子類不扮演任何角色。
TEST_CASE("純粹的 GameObject 子類不扮演任何角色") {
    struct Bare final : GameObject {
        Bare() : GameObject(nccu::engine::math::Vec2{0, 0}, nccu::engine::math::Rect{0, 0, 1, 1}) {}
    };
    Bare b;
    GameObject& g = b;
    CHECK(g.AsUpdatable()    == nullptr);
    CHECK(g.AsDrawable()     == nullptr);
    CHECK(g.AsInteractable() == nullptr);
}

// 靜態存取器回傳的指標確實能分派到具體覆寫。
TEST_CASE("靜態存取器回傳的指標確實能分派到具體覆寫") {
    // 不只是非 null：回傳的 IInteractable* 必須能呼叫到具體覆寫。
    // QuestFlagPickup 在 Interact 時會設定自己的旗標。
    Player p{nccu::engine::math::Vec2{0, 0}};
    QuestFlagPickup form{nccu::engine::math::Vec2{0, 0}, "Flag_RoleDispatch"};
    GameObject& g = form;
    REQUIRE(g.AsInteractable() != nullptr);
    CHECK_FALSE(p.HasFlag("Flag_RoleDispatch"));
    g.AsInteractable()->Interact(&p);
    CHECK(p.HasFlag("Flag_RoleDispatch"));
    CHECK_FALSE(form.IsActive());            // 拾取物如預期停用
}

// ForEachRole<IUpdatable> 只走訪會逐幀更新的物件。
TEST_CASE("ForEachRole<IUpdatable> 只走訪會逐幀更新的物件") {
    // 混合容器，正是場景容器的形狀。只有 Player 和 NPC 扮演 IUpdatable；
    // 雨傘／拾取物／消耗品必須被跳過（它們舊的 Update 是空的空操作）。
    std::vector<std::unique_ptr<GameObject>> objs;
    objs.push_back(std::make_unique<Player>(nccu::engine::math::Vec2{0, 0}));
    objs.push_back(std::make_unique<NPC>(nccu::engine::math::Vec2{0, 0},
                                         std::vector<std::string>{"x"}));
    objs.push_back(std::make_unique<TrueUmbrella>(nccu::engine::math::Vec2{0, 0}));
    objs.push_back(std::make_unique<HotPack>(nccu::engine::math::Vec2{0, 0}));
    objs.push_back(std::make_unique<CashPickup>(nccu::engine::math::Vec2{0, 0}, 10));

    int visited = 0;
    ForEachRole<IUpdatable>(objs, [&](IUpdatable&) { ++visited; });
    CHECK(visited == 2);                     // 只有 Player + NPC

    // 失效物件會被跳過（保留 mark-then-sweep 語意）。
    objs.front()->Deactivate();
    visited = 0;
    ForEachRole<IUpdatable>(objs, [&](IUpdatable&) { ++visited; });
    CHECK(visited == 1);                     // 現在只剩 NPC
}

// ── IMortal（戰鬥相關的角色骨架）──────────────────────
// Player 扮演 IMortal 角色；NPC／道具不扮演。
TEST_CASE("Player 扮演 IMortal 角色；NPC／道具不扮演") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    GameObject& gp = p;
    CHECK(gp.AsMortal() != nullptr);         // 玩家有生命值

    NPC n{nccu::engine::math::Vec2{0, 0}, std::vector<std::string>{"hi"}};
    GameObject& gn = n;
    CHECK(gn.AsMortal() == nullptr);         // 目前 NPC 沒有生命值

    TrueUmbrella u{nccu::engine::math::Vec2{0, 0}};
    GameObject& gu = u;
    CHECK(gu.AsMortal() == nullptr);         // 道具沒有生命值
}

// IMortal：TakeDamage 會降低 hp、裁切於 0、IsDead 翻轉。
TEST_CASE("IMortal：TakeDamage 降低 hp、裁切於 0、IsDead 翻轉") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    CHECK(p.Hp() == Player::kMaxHp);
    CHECK_FALSE(p.IsDead());
    p.TakeDamage(30);
    CHECK(p.Hp() == Player::kMaxHp - 30);
    CHECK_FALSE(p.IsDead());
    p.TakeDamage(-5);                        // 非正值忽略
    CHECK(p.Hp() == Player::kMaxHp - 30);
    p.TakeDamage(1000);                      // 超量傷害裁切於 0
    CHECK(p.Hp() == 0);
    CHECK(p.IsDead());
}

// ForEachRole<IMortal> 只走訪有生命值的實體，並分派傷害。
TEST_CASE("ForEachRole<IMortal> 只走訪有生命值的實體並分派傷害") {
    std::vector<std::unique_ptr<GameObject>> objs;
    objs.push_back(std::make_unique<Player>(nccu::engine::math::Vec2{0, 0}));
    objs.push_back(std::make_unique<NPC>(nccu::engine::math::Vec2{0, 0},
                                         std::vector<std::string>{"x"}));
    objs.push_back(std::make_unique<TrueUmbrella>(nccu::engine::math::Vec2{0, 0}));

    int visited = 0;
    ForEachRole<IMortal>(objs, [&](IMortal& m) { ++visited; m.TakeDamage(10); });
    CHECK(visited == 1);                     // 只有 Player 有生命值
    // 傷害確實透過分派到的 IMortal& 生效。
    CHECK(objs.front()->AsMortal()->Hp() == Player::kMaxHp - 10);
}

// ── GetCollisionLayer：碰撞層位 ────────────────
// GameObject 碰撞層位：預設為 0，可設定。
TEST_CASE("GameObject 碰撞層位：預設為 0，可設定") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    GameObject& g = p;
    CHECK(g.GetCollisionLayer() == 0);       // 預設層位
    g.SetCollisionLayer(3);
    CHECK(g.GetCollisionLayer() == 3);
}
