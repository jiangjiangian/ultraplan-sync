#include "game/controller/SimSystem.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "engine/core/Roles.h"
#include "game/controller/GameObjectQueries.h"
#include "game/world/Physics.h"
#include "game/gfx/Bounds.h"

namespace nccu {

// ── SurvivalSystem ───────────────────────────────────────────────────
// 企劃的降雨生存迴圈，每章皆生效且可致命，每幀 tick 一次。三種狀態：
//   在建築內         -> DrainRain          （每秒 -10，完全回復，真正的避雨所）
//   室外、持傘       -> ApplyRainSheltered （每秒 +1.5，仍可致命——傘只是減緩淋濕）
//   室外、無傘       -> ApplyRain          （每秒 +5，致命；Ch1 無傘可用，故 Ch1
//                                            降雨與先前逐位元相同）
// 只有在市集過場與各結局（安全、非遊戲性的狀態）下略過。當玩家中心位於建築觸發區內
// 時 CurrentBuildingName() 非空（於前一幀結尾刷新；在每秒 <=10 的速率下，這一幀的
// 延遲難以察覺且具確定性）。
void SurvivalSystem::Run(SimContext& ctx, float dt) {
    World& world = ctx.world;
    Player* player = world.GetPlayer();
    if (!player) return;
    const SemesterState ss = world.Semester().Current();
    const bool inEnding = ss == SemesterState::Ending_A ||
                          ss == SemesterState::Ending_B ||
                          ss == SemesterState::Ending_C;
    if (ss != SemesterState::Interlude_Market && !inEnding) {
        if (!world.CurrentBuildingName().empty())
            player->DrainRain(dt);                  // 完全回復
        else if (player->HasUmbrella())
            player->ApplyRainSheltered(dt, /*lethal=*/true);
        else
            player->ApplyRain(dt, /*lethal=*/true); // 完全曝露
    }
}

// ── MovementSystem ───────────────────────────────────────────────────
// 先存下玩家 tick 前的左上角座標，再 tick 每個 IUpdatable。ForEachRole 於編譯期把
// 角色對應到 AsUpdatable() 並略過其餘（不再帶 Update 的傘／拾取物根本不會被造訪
// ——與舊的空 no-op 呼叫等價）。存下的座標透過情境交給 CollisionSystem 做下方的
// 分軸解算。
void MovementSystem::Run(SimContext& ctx, float dt) {
    World& world = ctx.world;
    Player* player = world.GetPlayer();
    ctx.prevPlayerPos = player ? player->GetPosition()
                               : nccu::engine::math::Vec2{0.0f, 0.0f};
    ForEachRole<IUpdatable>(world.Objects(),
                            [dt](IUpdatable& u) { u.Update(dt); });
}

// ── CollisionSystem ──────────────────────────────────────────────────
// 先在 Update() 之後把玩家夾限至世界 AABB；再做分軸碰撞解算——地形遮罩加上每個
// BlocksMovement() 為真的物件碰撞盒，只在受阻的那一軸把玩家推回（讓玩家能沿牆斜滑）。
// 道具刻意不視為碰撞體。座標的浮點相等比較寫入精確保留（唯有解算值不同時才寫入，
// 以維持確定性輸出逐位元相同）。
void CollisionSystem::Run(SimContext& ctx, float /*dt*/) {
    using nccu::queries::ForEachActiveExcept;
    World& world = ctx.world;
    Player* player = world.GetPlayer();
    if (!player) return;

    const nccu::engine::math::Vec2 clamped =
        nccu::game::gfx::ClampToWorld(player->GetPosition(), ctx.playerSize,
                                ctx.worldSize);
    if (clamped.x != player->GetPosition().x ||
        clamped.y != player->GetPosition().y) {
        player->SetPosition(clamped);
    }

    ctx.frameColliders.clear();
    ForEachActiveExcept(world.Objects(), player, [&ctx](GameObject& o) {
        if (!o.BlocksMovement()) return;
        const nccu::engine::math::Vec2 p = o.GetPosition();
        ctx.frameColliders.push_back(
            nccu::engine::math::Rect{p.x, p.y, ctx.playerSize.x, ctx.playerSize.y});
    });
    const nccu::engine::math::Vec2 resolved = nccu::physics::ResolveMove(
        ctx.prevPlayerPos, player->GetPosition(), ctx.playerSize,
        ctx.frameColliders, &world.TerrainMask());
    if (resolved.x != player->GetPosition().x ||
        resolved.y != player->GetPosition().y) {
        player->SetPosition(resolved);
    }
}

// ── SpawnSystem ──────────────────────────────────────────────────────
// 先 tick 操場校慶的繞圈進度（僅 Ch3；其餘狀態為廉價 no-op），再依原順序跑四個
// 延遲生成：
//   Ch1 苦主之傘的「選擇後揭示」（Flag_SuitSeniorChoiceMade）
//   Ch2 散落筆記的「喚醒後揭示」（Flag_Bookworm）
//   Ch3 TrueUmbrella 的「線索後揭示」（Flag_KnowsUmbrellaLoc）
//   市集管理員之傘的歸還點（Ch2->Ch3 市集，持有借出的傘）
// 每個都在 World 內自我守門且只觸發一次，在其章節之外是廉價 no-op，故此階段每幀
// 無條件執行是安全的。World 維持純資料；由 system 負責每幀的 tick（MVC）。
void SpawnSystem::Run(SimContext& ctx, float /*dt*/) {
    World& world = ctx.world;
    world.UpdateSportsLap();
    world.MaybeSpawnChapter1VictimUmbrella();
    world.MaybeSpawnChapter2Notes();
    world.MaybeSpawnChapter3Umbrella();
    world.MaybeSpawnInterludeLibrarianReturn();
}

// ── SweepSystem ──────────────────────────────────────────────────────
// 幀末延遲刪除。標記後清除與「玩家懸空指標」防護皆位於 World::Sweep()。
void SweepSystem::Run(SimContext& ctx, float /*dt*/) {
    ctx.world.Sweep();
}

} // namespace nccu
