#ifndef OBSTACLES_H_
#define OBSTACLES_H_
#include <array>
#include <string_view>

/**
 * @file Obstacles.h
 * @brief 渲染期「不另疊 sprite」的建築白名單——其像素已烘焙進底圖。
 */

namespace nccu::obstacles {

/**
 * @brief 像素已烘焙進 worldmap_base.png（開放地面／廣場）的建築名單。
 *
 * View 對名單內的建築不另外合成 sprite。實體碰撞已不在此處——改由像素級可走
 * 遮罩 PNG（resources/assets/maps/collision_mask.png）載入成 CollisionMask；
 * 本名單純粹是渲染期的 sprite-skip。
 */
inline constexpr std::array<std::string_view, 2> kBuildingCollisionSkip = {
    "操場", "羅馬廣場"
};

} // namespace nccu::obstacles

#endif // OBSTACLES_H_
