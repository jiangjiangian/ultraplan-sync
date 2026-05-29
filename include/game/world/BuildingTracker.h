#ifndef BUILDING_TRACKER_H_
#define BUILDING_TRACKER_H_
#include "game/world/Buildings.h"
#include "engine/math/Vec2.h"
#include <limits>

/**
 * @file BuildingTracker.h
 * @brief 建築進入偵測：偵測玩家踏入建築觸發矩形的單邊轉換並發布進入事件。
 */

namespace nccu {

namespace detail {
/**
 * @brief 在 range 中所有觸發矩形包含 p 的建築裡，回傳矩形中心離 p 最近者。
 * @tparam Range 可逐一巡訪 buildings::Building 的範圍型別。
 * @param p     待測點（玩家中心）。
 * @param range 候選建築範圍。
 * @return 命中的建築指標；無任何矩形包含 p 時回傳 nullptr。
 *
 * Buildings.h 的部分觸發矩形天生重疊；以「最近中心」消歧（距離完全相等時再以名稱字
 * 典序打破平手）可得確定性結果，而非默默相依於容器順序。以範圍為模板參數，使消歧邏
 * 輯能對合成測試資料做單元測試，與實際（由 Tiled 重新產生的）校園布局解耦。
 */
template <typename Range>
const buildings::Building* NearestContaining(nccu::engine::math::Vec2 p, const Range& range) {
    const buildings::Building* found = nullptr;
    float bestDistSq = std::numeric_limits<float>::max();
    for (const auto& b : range) {
        if (!b.triggerRect.Contains(p)) continue;
        const float cx = b.triggerRect.x + b.triggerRect.width  * 0.5f;
        const float cy = b.triggerRect.y + b.triggerRect.height * 0.5f;
        const float dx = cx - p.x;
        const float dy = cy - p.y;
        const float d2 = dx * dx + dy * dy;
        if (d2 < bestDistSq ||
            (found != nullptr && d2 == bestDistSq && b.name < found->name)) {
            bestDistSq = d2;
            found = &b;
        }
    }
    return found;
}
} // namespace detail

/**
 * @brief 建築進入的單邊轉換偵測器。
 *
 * 玩家踏入某建築觸發矩形的第一幀，於事件匯流排發布 EventType::EnteredBuilding 並帶
 * 上建築名稱；停留於同一建築的後續幀保持靜默。走進空地會清除 Current() 但不發事件。
 */
class BuildingTracker {
public:
    /**
     * @brief 以玩家中心點更新追蹤，必要時發布進入事件。
     * @param playerCenter 玩家碰撞盒中心的世界座標。
     * @return 目前所在建築指標；不在任何建築內時為 nullptr。
     */
    const buildings::Building* Update(nccu::engine::math::Vec2 playerCenter);

    /// @brief 取得目前所在建築（不在任何建築內時為 nullptr）。
    [[nodiscard]] const buildings::Building* Current() const noexcept { return current_; }

private:
    const buildings::Building* current_{nullptr}; ///< 目前所在建築；nullptr 表空地
};

} // namespace nccu

#endif // BUILDING_TRACKER_H_
