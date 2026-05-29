#ifndef WORLD_CONFIG_H_
#define WORLD_CONFIG_H_

/**
 * @file WorldConfig.h
 * @brief 世界尺寸與玩家碰撞盒尺寸的編譯期常數。
 */

namespace world {

/// @brief 世界地圖邊長（正方形）。對應 View 實際渲染的 worldmap_base.png；
///        若該資產被重新調整尺寸，須同步更新。
inline constexpr float kSize = 2048.0f;

/// @brief 玩家碰撞盒寬度，須與 Player.cpp 的 24×24 hit-box 一致。
inline constexpr float kPlayerWidth  = 24.0f;
/// @brief 玩家碰撞盒高度，須與 Player.cpp 的 24×24 hit-box 一致。
inline constexpr float kPlayerHeight = 24.0f;

} // namespace world

#endif // WORLD_CONFIG_H_
