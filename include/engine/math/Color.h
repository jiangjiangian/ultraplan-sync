#ifndef GFX_COLOR_H_
#define GFX_COLOR_H_
#include <cstdint>   // std::uint8_t — 每通道 0..255

/**
 * @file Color.h
 * @brief RGBA8 顏色值型別與一組常用顏色常數；不依賴 raylib。
 */

namespace nccu::engine::math {

/**
 * @brief 每通道 8 位元的 RGBA 顏色，預設不透明黑。
 *
 * 引擎內部的顏色載體；與 raylib 的 ::Color 在渲染層欄位對映，使上層無須引入
 * raylib 標頭即可指定顏色。
 */
struct Color {
    std::uint8_t r{0};     ///< 紅
    std::uint8_t g{0};     ///< 綠
    std::uint8_t b{0};     ///< 藍
    std::uint8_t a{255};   ///< Alpha（255 = 不透明）

    /**
     * @brief 取得僅替換 Alpha 的同色副本。
     * @param[in] newA 新的 Alpha 值。
     * @return RGB 不變、Alpha 為 newA 的新顏色。
     */
    constexpr Color WithAlpha(std::uint8_t newA) const noexcept {
        return Color{r, g, b, newA};
    }
};

/** @brief 逐通道相等比較。 */
constexpr bool operator==(Color a, Color b) noexcept {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
/** @brief 逐通道不等比較。 */
constexpr bool operator!=(Color a, Color b) noexcept { return !(a == b); }

/** @brief 常用顏色常數（沿用 raylib 的調色值，方便對照）。 */
namespace Colors {
inline constexpr Color Black     {  0,   0,   0, 255};
inline constexpr Color White     {255, 255, 255, 255};
inline constexpr Color RayWhite  {245, 245, 245, 255};
inline constexpr Color DarkGray  { 80,  80,  80, 255};
inline constexpr Color Blue      {  0, 121, 241, 255};
inline constexpr Color Red       {230,  41,  55, 255};
inline constexpr Color Green     {  0, 228,  48, 255};
inline constexpr Color Yellow    {253, 249,   0, 255};
inline constexpr Color Gold      {255, 203,   0, 255};
inline constexpr Color Magenta   {255,   0, 255, 255};
}

} // namespace nccu::engine::math

#endif // GFX_COLOR_H_
