#include "game/world/CollisionMask.h"
#include "game/gfx/MaskLoader.h"

namespace nccu {

CollisionMask LoadTerrainMask() {
    return nccu::game::gfx::LoadCollisionMask(
        "resources/assets/maps/collision_mask.png",
        "resources/assets/maps/collision_mask_base.png");
}

} // namespace nccu
