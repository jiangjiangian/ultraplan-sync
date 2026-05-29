#ifndef SIM_SYSTEM_H_
#define SIM_SYSTEM_H_
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include <vector>

namespace nccu {

class World;

/**
 * @file SimSystem.h
 * @brief 每幀模擬管線：ISystem 介面、其共用的 SimContext，以及依管線順序排列
 *        的各具體階段（Survival/Movement/Collision/Spawn/Sweep）。
 *
 * GameController::Update 原本把每個模擬階段內嵌成一個龐大的 god-method。現把每個
 * 空間上獨立、可重用的階段拆成單一職責的 ISystem，由 Controller 以「完全相同」的
 * 順序執行（故確定性的序列化輸出維持逐位元相同）。
 *
 * MVC 純度：system 只操作模型（World&／Player&），不讀輸入裝置、不呼叫 raylib、
 * 不渲染——那些分別留在 Controller（輸入）與 View（渲染）。各畫面的輸入區塊
 * （結局選單／暫停／對話推進／背包）與 E 鍵互動派發「不是」system，因其讀取輸入，
 * 故留在 Controller 層。
 */

/**
 * @brief 串接整條管線、僅供一幀使用的「純模型」情境物件。
 *
 * 打包了 World、碰撞階段所需的兩個幾何常數、一個重用的暫存碰撞盒清單（使每幀的
 * vector 不必重新配置），以及由 MovementSystem 交給 CollisionSystem 的前一幀玩家
 * 座標。不含 raylib、不含輸入——純資料。
 */
struct SimContext {
    World&                          world;        ///< 本幀操作的世界
    nccu::engine::math::Vec2                 worldSize;    ///< 世界邊界尺寸（像素）
    nccu::engine::math::Vec2                 playerSize;   ///< 玩家碰撞盒尺寸（像素）
    std::vector<nccu::engine::math::Rect>&   frameColliders;  ///< 重用的暫存碰撞盒清單
    /// 由 MovementSystem 設定（本幀物件 Update tick 之前的玩家左上角），由
    /// CollisionSystem 讀取以做分軸解算。放在情境中使兩階段保持解耦型別，又能保留
    /// 原內嵌碼「前一位置 -> 解算」的精確交接。
    nccu::engine::math::Vec2                 prevPlayerPos{0.0f, 0.0f};
};

/**
 * @brief 一個有序的模擬階段（管線元素）。
 *
 * Run() 將模型推進 dt 秒。具體階段以 final 實作此介面。
 */
struct ISystem {
    virtual ~ISystem() = default;
    /**
     * @brief 將模型推進一個時間步。
     * @param[in,out] ctx 本幀共用的模擬情境。
     * @param[in]     dt  幀間隔（秒）。
     */
    virtual void Run(SimContext& ctx, float dt) = 0;
};

// ── 各具體階段，依管線順序排列 ────────────────────────────────────────

/**
 * @brief 降雨生存的累積／回復階段。
 *
 * 三種狀態：在建築內 -> DrainRain（每秒 -10，完全回復）；室外持傘 ->
 * ApplyRainSheltered（每秒 +1.5，仍可致命）；室外無傘 -> ApplyRain（每秒 +5，
 * 致命）。在市集過場與各結局狀態下略過。
 */
struct SurvivalSystem final : ISystem {
    void Run(SimContext& ctx, float dt) override;
};

/**
 * @brief 移動階段：先把玩家 tick 前座標存入 ctx.prevPlayerPos，再 tick 每個
 *        IUpdatable 物件（ForEachRole<IUpdatable>）。
 *
 * ISP 角色派發不變——只有扮演 IUpdatable 角色的物件會移動。
 */
struct MovementSystem final : ISystem {
    void Run(SimContext& ctx, float dt) override;
};

/**
 * @brief 碰撞階段：玩家 AABB 解算——先夾限至世界邊界，再對每個 BlocksMovement()
 *        物件的碰撞盒與地形遮罩做分軸解算。道具刻意不視為碰撞體。
 */
struct CollisionSystem final : ISystem {
    void Run(SimContext& ctx, float dt) override;
};

/**
 * @brief 生成階段：每幀的延遲生成——先 tick 操場繞圈進度，再依序跑四個自我守門的
 *        MaybeSpawn（Ch1 苦主之傘／Ch2 筆記／Ch3 雨傘／市集管理員歸還）。
 *
 * 每個都自我守門，在其章節之外是廉價的 no-op，故可每幀無條件執行。
 */
struct SpawnSystem final : ISystem {
    void Run(SimContext& ctx, float dt) override;
};

/**
 * @brief 清除階段（終端）：幀末的標記後清除（延遲刪除）。
 *
 * 委派給 World::Sweep()——保留為獨立階段，使管線擁有完整有序的一幀，且未來可在
 * 清除前後插入新 system。
 */
struct SweepSystem final : ISystem {
    void Run(SimContext& ctx, float dt) override;
};

} // namespace nccu

#endif // SIM_SYSTEM_H_
