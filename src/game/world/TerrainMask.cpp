#include "game/world/CollisionMask.h"
#include "game/gfx/MaskLoader.h"

namespace nccu {

CollisionMask LoadTerrainMask() {
    return gfx::LoadCollisionMask(
        "resources/assets/maps/collision_mask.png",
        "resources/assets/maps/collision_mask_base.png");
}

} // namespace nccu
