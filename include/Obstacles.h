#ifndef OBSTACLES_H_
#define OBSTACLES_H_
#include <array>
#include <string_view>

namespace nccu::obstacles {

// Buildings whose pixels are baked into worldmap_base.png (open ground
// / plaza), so View.cpp composites no sprite for them. Physical
// collision no longer lives here — it is a pixel-accurate walkability
// PNG (resources/assets/maps/collision_mask.png) loaded into a
// CollisionMask; this list is purely a render-time sprite-skip.
inline constexpr std::array<std::string_view, 2> kBuildingCollisionSkip = {
    "操場", "羅馬廣場"
};

} // namespace nccu::obstacles

#endif // OBSTACLES_H_
