#ifndef GFX_WALK_CYCLE_H_
#define GFX_WALK_CYCLE_H_
#include "engine/math/Vec2.h"
#include <array>
#include <cmath>
#include <cstddef>

/**
 * @file WalkCycle.h
 * @brief 純標頭的 Pipoya 行走圖格運算——「96x128 圖中該貼哪一格 32x32」的
 *        唯一受測真實來源。
 *
 * Player 與 NPC 共用同一套運算，讓遊蕩／校慶賽跑的 NPC 與玩家的步態完全一致。
 * 函式僅依「動畫步進 + 朝向向量」計算，不碰 raylib／GL／模擬狀態，因此圖格選擇
 * 可在無 GL context 的無頭測試中單元測試（NPC::Render 的貼圖本身則不行——無頭
 * 環境中 Texture::IsValid() 為 false）。
 */
namespace nccu::game::gfx {

/// @name Pipoya 行走圖規格
/// 單格 32x32；整張圖為 3 欄（行走影格）× 4 列（朝向）。行走序列為
/// idle(1) → 左腳(0) → idle(1) → 右腳(2)，以 4 步計數讓兩個跨步影格在
/// idle 影格間來回，看起來像自然的踏步。
///@{
inline constexpr int   kPipoyaCell        = 32;
inline constexpr float kWalkFrameDuration  = 0.15f;  ///< 每步秒數
inline constexpr std::array<int, 4> kWalkColumns = {1, 0, 1, 2};
///@}

/**
 * @brief 取動畫步進 `s` 對應的影格欄位。
 * @param s 動畫步進（任意整數，內部摺回 [0,4)）。
 * @return 影格欄位索引。
 *
 * 步進 0 是 idle 欄，故剛重設／靜止的 sprite 顯示 idle 姿勢；移動（前進）與
 * 靜止（步進 0）兩種情況共用此函式。
 */
[[nodiscard]] constexpr int WalkColumn(int s) noexcept {
    const std::size_t i = static_cast<std::size_t>(((s % 4) + 4) % 4);
    return kWalkColumns[i];
}

/**
 * @brief 由朝向向量取 Pipoya 朝向列：0=下、1=左、2=右、3=上。
 * @param facing 朝向向量。
 * @return 朝向列索引（0..3）。
 *
 * 取絕對值較大的軸為主（|x| vs |y|）；相等時偏向垂直軸，使完美對角仍面向上／
 * 下。零向量回傳列 0（下）——即標準的「面向鏡頭」靜止姿勢。
 */
[[nodiscard]] constexpr int WalkRowForFacing(nccu::engine::math::Vec2 facing) noexcept {
    const float ax = facing.x < 0.0f ? -facing.x : facing.x;
    const float ay = facing.y < 0.0f ? -facing.y : facing.y;
    if (ax > ay) return facing.x < 0.0f ? 1 : 2;
    return facing.y < 0.0f ? 3 : 0;
}

} // namespace nccu::game::gfx

#endif // GFX_WALK_CYCLE_H_
