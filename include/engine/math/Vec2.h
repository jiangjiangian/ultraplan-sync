#ifndef GFX_VEC2_H_
#define GFX_VEC2_H_
#include <cmath>   // std::sqrt — 供 Length() 計算向量長度

/**
 * @file Vec2.h
 * @brief 二維浮點向量：座標、位移與方向運算的最小值型別。
 */

namespace nccu::engine::math {

/**
 * @brief 二維向量／點，採彙總初始化（aggregate）的純資料型別。
 *
 * 不依賴 raylib，作為引擎內部位置、速度、尺寸的通用載體；與 raylib 的
 * ::Vector2 在渲染層做欄位對映即可。
 */
struct Vec2 {
    float x{0.0f};   ///< x 分量
    float y{0.0f};   ///< y 分量

    /** @brief 取得向量長度（歐氏範數）。@return sqrt(x²+y²)。 */
    float Length() const noexcept { return std::sqrt(x * x + y * y); }

    /**
     * @brief 取得單位化後的同向向量。
     * @return 長度為 1 的同向向量；長度近乎 0 時回傳零向量以避免除以 0。
     */
    Vec2 Normalized() const noexcept {
        float len = Length();
        if (len < 1e-6f) return Vec2{0.0f, 0.0f};
        return Vec2{x / len, y / len};
    }
};

/** @brief 逐分量相加。 */
constexpr Vec2 operator+(Vec2 a, Vec2 b) noexcept { return Vec2{a.x + b.x, a.y + b.y}; }
/** @brief 逐分量相減。 */
constexpr Vec2 operator-(Vec2 a, Vec2 b) noexcept { return Vec2{a.x - b.x, a.y - b.y}; }
/** @brief 向量乘以純量。 */
constexpr Vec2 operator*(Vec2 a, float s) noexcept { return Vec2{a.x * s, a.y * s}; }
/** @brief 純量乘以向量（交換律重載）。 */
constexpr Vec2 operator*(float s, Vec2 a) noexcept { return a * s; }

} // namespace nccu::engine::math

#endif // GFX_VEC2_H_
