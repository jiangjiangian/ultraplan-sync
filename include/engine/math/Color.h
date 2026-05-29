#ifndef GFX_COLOR_H_
#define GFX_COLOR_H_
#include <cstdint>

namespace nccu::engine::math {

struct Color {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    std::uint8_t a{255};

    constexpr Color WithAlpha(std::uint8_t newA) const noexcept {
        return Color{r, g, b, newA};
    }
};

constexpr bool operator==(Color a, Color b) noexcept {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
constexpr bool operator!=(Color a, Color b) noexcept { return !(a == b); }

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
