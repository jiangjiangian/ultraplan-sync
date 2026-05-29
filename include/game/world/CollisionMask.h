#ifndef COLLISION_MASK_H_
#define COLLISION_MASK_H_
#include "engine/math/Vec2.h"
#include <cstdint>
#include <vector>

/**
 * @file CollisionMask.h
 * @brief 烘焙地形的像素級可走遮罩，以及載入正規遮罩的工廠函式。
 */

namespace nccu {

/**
 * @brief 烘焙地形的逐像素可走格點（建築牆基、河流、手繪樹木／花圃／校園外牆）。
 *
 * 每個世界像素一個 byte：1 = 實心、0 = 可走。PNG 可用任意影像編輯器繪製
 * （實心的判定慣例為「非純白且非全透明」），並透過
 * nccu::game::gfx::LoadCollisionMask 載入，使 raylib 僅侷限於 include/gfx/。
 * 本型別不依賴 raylib，因此 Physics.h 的 AABB 解算器可直接取用。
 *
 * 空遮罩（Width()==0）回報每個盒子皆可走——這是找不到遮罩 PNG 時的優雅降級狀態。
 */
class CollisionMask {
public:
    CollisionMask() = default;
    CollisionMask(int w, int h, std::vector<std::uint8_t> solid)
        : w_(w), h_(h), solid_(std::move(solid)) {}

    int  Width()  const noexcept { return w_; }
    int  Height() const noexcept { return h_; }
    bool Empty()  const noexcept { return w_ <= 0 || h_ <= 0; }

    bool Solid(int px, int py) const noexcept {
        if (Empty()) return false;
        if (px < 0) px = 0; else if (px >= w_) px = w_ - 1;
        if (py < 0) py = 0; else if (py >= h_) py = h_ - 1;
        return solid_[static_cast<std::size_t>(py) * w_ + px] != 0;
    }

    /**
     * @brief AABB 覆蓋範圍內是否有任一像素為實心。
     *
     * 盒子以 kStep 像素的格點掃描（永遠含遠端邊界），讓比玩家還薄的牆絕不會
     * 從取樣間隙漏過——像素精確遮罩的全部意義，就是引擎要剛好擋在被畫出來的東西上。
     */
    bool BlockedBox(float bx, float by, float bw, float bh) const noexcept {
        if (Empty()) return false;
        constexpr float kStep = 4.0f;
        const float x1 = bx + bw, y1 = by + bh;
        for (float sy = by;; sy += kStep) {
            if (sy > y1) sy = y1;
            for (float sx = bx;; sx += kStep) {
                if (sx > x1) sx = x1;
                if (Solid(static_cast<int>(sx), static_cast<int>(sy)))
                    return true;
                if (sx >= x1) break;
            }
            if (sy >= y1) break;
        }
        return false;
    }

    bool BlockedBox(nccu::engine::math::Vec2 pos, nccu::engine::math::Vec2 size) const noexcept {
        return BlockedBox(pos.x, pos.y, size.x, size.y);
    }

private:
    int                       w_{0};
    int                       h_{0};
    std::vector<std::uint8_t> solid_;
};

/**
 * @brief 載入正規地形遮罩。
 * @return 對應的可走遮罩；資產缺失時退化為空遮罩（每個盒子皆可走）。
 *
 * 優先載入手繪的 resources/assets/maps/collision_mask.png，缺檔時退回工具產生的
 * collision_mask_base.png（僅建築與河流、無裝飾物）。實作於 TerrainMask.cpp——唯一
 * 獲准經由 nccu::game::gfx::LoadCollisionMask 取用 raylib 的非 gfx 編譯單元，使模型層
 * （World）維持不相依 raylib。
 */
CollisionMask LoadTerrainMask();

} // namespace nccu

#endif // COLLISION_MASK_H_
