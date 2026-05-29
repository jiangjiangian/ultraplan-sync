#include "game/world/CollisionMask.h"
#include "game/gfx/MaskLoader.h"

/**
 * @file TerrainMask.cpp
 * @brief LoadTerrainMask 實作：唯一允許經由 gfx 載入器取用 raylib 的非 gfx 編譯單元，
 *        使模型層（World）維持不相依 raylib。
 */

namespace nccu {

CollisionMask LoadTerrainMask() {
    return nccu::game::gfx::LoadCollisionMask(
        "resources/assets/maps/collision_mask.png",
        "resources/assets/maps/collision_mask_base.png");
}

} // namespace nccu
