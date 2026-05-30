#include "doctest/doctest.h"
#include "game/controller/SimSystem.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/entities/NPC.h"
#include "engine/math/Vec2.h"
#include "engine/math/Rect.h"

#include <cstdint>
#include <memory>
#include <vector>

/**
 * @file test_sim_systems.cpp
 * @brief 驗證從 GameController::Update 抽出的「模型推進」管線各階段（ISystem）：
 *        SurvivalSystem 三向雨量分支、MovementSystem 擷取前一格位置、CollisionSystem
 *        夾限出界玩家、SpawnSystem 在非對應章節為安全 no-op、SweepSystem／World::Sweep
 *        清除失效物件並維護 Player 快取。
 *
 * 每個 ISystem 只操作 World&/Player&——無輸入、無 raylib——故無 GL 的 headless World
 *（loadSprites=false）可直接驅動，並在隔離下驗證各階段的單一職責。
 */

using nccu::CollisionSystem;
using nccu::MovementSystem;
using nccu::SimContext;
using nccu::SpawnSystem;
using nccu::SurvivalSystem;
using nccu::SweepSystem;
using nccu::SemesterState;
using nccu::World;

namespace {

// 在 World 上建立全新的 SimContext，對應 GameController 每格的建構方式（兩個幾何常數 +
// 一份重複使用的暫存 collider 清單）。
struct Fixture {
    World                          w{"", /*loadSprites=*/false};
    std::vector<nccu::engine::math::Rect>   colliders;
    SimContext ctx() {
        return SimContext{w, nccu::engine::math::Vec2{2048.0f, 2048.0f},
                          nccu::engine::math::Vec2{24.0f, 24.0f}, colliders, {}};
    }
};

} // namespace

// ── SurvivalSystem：三向雨量分支 ───────────────────────────
// 室外、無傘時累積雨量（Ch1 預設）。
TEST_CASE("SurvivalSystem：室外無傘時累積雨量（Ch1 預設）") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    REQUIRE(p != nullptr);
    REQUIRE(p->HasUmbrella() == false);
    REQUIRE(f.w.CurrentBuildingName().empty());
    const float before = p->GetRainMeter();
    SurvivalSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 1.0f);                       // 曝露 1 秒
    CHECK(p->GetRainMeter() > before);      // +5 u/s 累積
}

// 室外、有傘時以較低速率累積。
TEST_CASE("SurvivalSystem：室外有傘時以較低速率累積") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    p->SetHasUmbrella(true);
    const float before = p->GetRainMeter();
    SurvivalSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 1.0f);
    // 有遮蔽的累積為正，但慢於曝露時的 +5。
    CHECK(p->GetRainMeter() > before);
    CHECK(p->GetRainMeter() - before < 5.0f);
}

// 在建築室內會排掉雨量。
TEST_CASE("SurvivalSystem：在建築室內會排掉雨量") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    p->ApplyRain(2.0f, /*lethal=*/false);   // 先淋濕，才有可排掉的量
    const float soaked = p->GetRainMeter();
    REQUIRE(soaked > 0.0f);
    f.w.CurrentBuildingName() = "圖書館";    // 進入室內遮蔽
    SurvivalSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 0.5f);
    CHECK(p->GetRainMeter() < soaked);      // -10 u/s 回復
}

// 市集 interlude 期間不進行雨量計算。
TEST_CASE("SurvivalSystem：市集 interlude 期間不進行雨量計算") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    f.w.Semester().Transition(SemesterState::Interlude_Market);
    const float before = p->GetRainMeter();
    SurvivalSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 5.0f);                        // 時間雖長但被略過
    CHECK(p->GetRainMeter() == doctest::Approx(before));
}

// ── MovementSystem：擷取前一格位置 + 推進物件 ─────────────────
// MovementSystem 將推進前的玩家位置擷取進 ctx。
TEST_CASE("MovementSystem 將推進前的玩家位置擷取進 ctx") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    p->SetPosition(nccu::engine::math::Vec2{123.0f, 456.0f});
    MovementSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 0.016f);
    CHECK(c.prevPlayerPos.x == doctest::Approx(123.0f));
    CHECK(c.prevPlayerPos.y == doctest::Approx(456.0f));
}

// ── CollisionSystem：將玩家夾限進世界 AABB ───────────
// CollisionSystem 把出界的玩家夾回世界內。
TEST_CASE("CollisionSystem 把出界的玩家夾回世界內") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    // 將玩家放到右下邊界之外；夾限應在每個軸把它拉回 worldSize - playerSize。
    p->SetPosition(nccu::engine::math::Vec2{9000.0f, 9000.0f});
    CollisionSystem sys;
    SimContext c = f.ctx();
    c.prevPlayerPos = p->GetPosition();      // Movement 原本會設定此值
    sys.Run(c, 0.016f);
    CHECK(p->GetPosition().x <= 2048.0f - 24.0f);
    CHECK(p->GetPosition().y <= 2048.0f - 24.0f);
    CHECK(p->GetPosition().x >= 0.0f);
    CHECK(p->GetPosition().y >= 0.0f);
}

// ── SpawnSystem：在對應章節以外為廉價 no-op，絕不崩潰 ────
// 全新 Ch1（未設任何旗標）下 SpawnSystem 為安全 no-op。
TEST_CASE("SpawnSystem 在 Ch1 預設下為安全 no-op（無延遲生成武裝）") {
    Fixture f;
    const std::size_t before = f.w.Objects().size();
    SpawnSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 0.016f);
    // 全新 Ch1 下四個延遲生成皆未觸發（無旗標），且 lap tick 在 Ch3 以外為 no-op——故
    // 名單不變。重點是整個階段不論章節都正確且不崩潰。
    CHECK(f.w.Objects().size() == before);
}

// ── SweepSystem / World::Sweep ──────────────────────────────────────
// SweepSystem 移除已停用的物件，並保持 Player 在名單最前端。
TEST_CASE("SweepSystem 移除已停用的物件，並保持 Player 在名單最前端") {
    Fixture f;
    Player* p = f.w.GetPlayer();
    REQUIRE(p != nullptr);
    REQUIRE(f.w.Objects().front().get() == static_cast<GameObject*>(p));
    // 在尾端加入一個已死亡的 NPC，再 sweep。
    auto npc = std::make_unique<NPC>(nccu::engine::math::Vec2{50, 50},
                                     std::vector<std::string>{"hi"});
    npc->Deactivate();
    f.w.Objects().push_back(std::move(npc));
    const std::size_t before = f.w.Objects().size();
    SweepSystem sys;
    SimContext c = f.ctx();
    sys.Run(c, 0.016f);
    CHECK(f.w.Objects().size() == before - 1);          // 死亡 NPC 被回收
    CHECK(f.w.Objects().front().get() ==                 // 最前端仍是 Player
          static_cast<GameObject*>(p));
    CHECK(f.w.GetPlayer() == p);                         // 快取仍有效
}

// Player 死亡時 World::Sweep 會清除快取的 player 指標。
TEST_CASE("Player 死亡時 World::Sweep 會清除快取的 player 指標") {
    World w{"", /*loadSprites=*/false};
    Player* p = w.GetPlayer();
    REQUIRE(p != nullptr);
    // ctor 也會生成一批章節 NPC，故世界非空——玩家死亡的契約是：在 erase 釋放物件前先
    // 清除快取，且玩家物件本身消失。
    const std::size_t before = w.Objects().size();
    // 在 sweep 釋放之前，先把玩家位址存成整數（比較已釋放指標的「值」屬未定義行為，整數
    // 複本則永遠定義良好），這樣「無倖存者是舊玩家」的檢查永不碰觸無效指標。
    const std::uintptr_t deadAddr =
        reinterpret_cast<std::uintptr_t>(static_cast<GameObject*>(p));
    p->Deactivate();                  // 本格標記玩家死亡
    w.Sweep();
    CHECK(w.GetPlayer() == nullptr);  // 在 erase 釋放前快取已清除
    CHECK(w.Objects().size() == before - 1);  // 玩家物件已移除
    // 無任何倖存物件位於（已釋放的）舊玩家位址。
    for (const auto& o : w.Objects())
        CHECK(reinterpret_cast<std::uintptr_t>(o.get()) != deadAddr);
}

// 無任何物件死亡時 World::Sweep 為 no-op。
TEST_CASE("無任何物件死亡時 World::Sweep 為 no-op") {
    World w{"", /*loadSprites=*/false};
    const std::size_t before = w.Objects().size();
    Player* p = w.GetPlayer();
    w.Sweep();
    CHECK(w.Objects().size() == before);
    CHECK(w.GetPlayer() == p);
}
