#pragma once

namespace world {

// Worldmap is square. Tracks resources/assets/maps/worldmap.png — keep
// in sync if that asset is ever resized.
inline constexpr float kSize = 2048.0f;

// Mirrors the 24×24 hit-box in Player.cpp.
inline constexpr float kPlayerWidth  = 24.0f;
inline constexpr float kPlayerHeight = 24.0f;

} // namespace world
