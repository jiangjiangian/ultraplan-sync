#pragma once
#include <cmath>

namespace nccu::gfx {

struct Vec2 {
    float x{0.0f};
    float y{0.0f};

    float Length() const noexcept { return std::sqrt(x * x + y * y); }

    Vec2 Normalized() const noexcept {
        float len = Length();
        if (len < 1e-6f) return Vec2{0.0f, 0.0f};
        return Vec2{x / len, y / len};
    }
};

constexpr Vec2 operator+(Vec2 a, Vec2 b) noexcept { return Vec2{a.x + b.x, a.y + b.y}; }
constexpr Vec2 operator-(Vec2 a, Vec2 b) noexcept { return Vec2{a.x - b.x, a.y - b.y}; }
constexpr Vec2 operator*(Vec2 a, float s) noexcept { return Vec2{a.x * s, a.y * s}; }
constexpr Vec2 operator*(float s, Vec2 a) noexcept { return a * s; }

} // namespace nccu::gfx
