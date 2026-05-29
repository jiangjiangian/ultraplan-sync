#ifndef PHYSICS_H_
#define PHYSICS_H_
#include "engine/math/Vec2.h"
#include "engine/math/Rect.h"
#include "game/world/CollisionMask.h"
#include <vector>

/**
 * @file Physics.h
 * @brief 軸分離的 AABB 移動解算：玩家與動態碰撞體及靜態地形遮罩的滑牆判定。
 */

namespace nccu::physics {

/**
 * @brief 軸分離的 AABB 移動解算器。
 * @param prev       玩家本幀更新前的左上角座標。
 * @param desired    玩家本幀想移動到的左上角座標。
 * @param playerSize 玩家 AABB 尺寸。
 * @param colliders  本幀的動態碰撞體清單（其他角色的碰撞盒）；以 const 參考傳入，
 *                   讓呼叫端可每幀重建。
 * @param mask       可選的靜態地形可走遮罩（建築牆基、河流、手繪樹木／花圃／外牆）；
 *                   傳 nullptr 表略過地形（無遮罩的 NPC 路徑、單元測試）。
 * @return 玩家實際應停留的左上角座標。
 *
 * 策略：先單獨嘗試 X 位移，若結果 AABB 與任一碰撞體重疊則放棄（X 維持 prev.x）；再
 * 從 X 解算後的位置嘗試 Y 位移。如此產生經典 JRPG 的「沿牆滑行」手感：斜向走入牆角
 * 時僅在未被擋住的軸上移動。
 */
inline nccu::engine::math::Vec2 ResolveMove(nccu::engine::math::Vec2 prev,
                             nccu::engine::math::Vec2 desired,
                             nccu::engine::math::Vec2 playerSize,
                             const std::vector<nccu::engine::math::Rect>& colliders,
                             const CollisionMask* mask = nullptr) {
    auto overlapsAny = [&](float x, float y) -> bool {
        const nccu::engine::math::Rect aabb{x, y, playerSize.x, playerSize.y};
        for (const auto& c : colliders) {
            if (aabb.Intersects(c)) return true;
        }
        return mask && mask->BlockedBox(x, y, playerSize.x, playerSize.y);
    };

    // 脫困模式：若 prev 已與某物重疊（出生在碰撞體上、NPC 走進玩家等），下方的軸測
    // 試會永遠失敗而使玩家卡死。此時直接放行到 desired——假定下一幀的 prev 已淨空，
    // 正常阻擋隨即恢復。
    if (overlapsAny(prev.x, prev.y)) return desired;

    nccu::engine::math::Vec2 out = prev;
    if (!overlapsAny(desired.x, prev.y)) out.x = desired.x;
    if (!overlapsAny(out.x, desired.y))  out.y = desired.y;
    return out;
}

} // namespace nccu::physics

#endif // PHYSICS_H_
