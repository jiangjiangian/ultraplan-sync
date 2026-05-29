#ifndef WORLD_CONFIG_H_
#define WORLD_CONFIG_H_

namespace world {

// Worldmap is square. Tracks resources/assets/maps/worldmap_base.png (the
// texture View actually renders) — keep in sync if that asset is resized.
inline constexpr float kSize = 2048.0f;

// Mirrors the 24×24 hit-box in Player.cpp.
inline constexpr float kPlayerWidth  = 24.0f;
inline constexpr float kPlayerHeight = 24.0f;

} // namespace world

#endif // WORLD_CONFIG_H_
