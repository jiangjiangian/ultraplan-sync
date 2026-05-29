#ifndef INTERLUDE_EXIT_H_
#define INTERLUDE_EXIT_H_
#include "engine/math/Vec2.h"

/**
 * @file InterludeExit.h
 * @brief 幕間市集的進入點與南側出口觸發區幾何，以及純幾何的出口判定述詞。
 */

namespace nccu {

// 幕間市集（四維道常態市集）沿用單一校園平面。市集腳本沒有對話用的 NPC 看板區段，
// 因此出口採設計核可的「走出市集南端觸發區」，而非看板 NPC 選單——純資料驅動、無硬
// 編對白、不需動到作者管理的內容檔。
//
// 進入幕間時玩家會被重置到 kInterludeEntry（避開出口帶，免得在南側結束的章節一進來
// 就被彈出去）。往南走進出口區即觸發離場旗標，由既有的章節閘門消費並轉移到
// SemesterStateMachine::InterludeReturnTo()。下方為純幾何，使判定述詞能在不依賴 GUI
// 與烘焙地形遮罩的情況下做單元測試；該帶實際是否可步行可達則屬人工驗證項目。

/// @brief 進入幕間時玩家被重置到的座標（避開南側出口帶）。
inline constexpr nccu::engine::math::Vec2 kInterludeEntry{500.0f, 1500.0f};

// 橫跨可步行四維道走廊的南側出口帶。玩家在 y=1860 出生、路人在此路上約於 y~1880
// 行走，故 y>=1900 是已知可步行地面再往南的一條帶。
inline constexpr float kInterludeExitMinX = 150.0f;   ///< 出口帶左界 x
inline constexpr float kInterludeExitMaxX = 1950.0f;  ///< 出口帶右界 x
inline constexpr float kInterludeExitMinY = 1900.0f;  ///< 出口帶上界 y
inline constexpr float kInterludeExitMaxY = 2048.0f;  ///< 出口帶下界 y（等於 world::kSize）

/**
 * @brief 判斷某中心點是否落在幕間南側出口觸發區內。
 * @param centre 待測中心點（通常為玩家碰撞盒中心）。
 * @return 點落在出口帶矩形內回傳 true。
 */
[[nodiscard]] inline bool InInterludeExitZone(nccu::engine::math::Vec2 centre) noexcept {
    return centre.x >= kInterludeExitMinX && centre.x <= kInterludeExitMaxX &&
           centre.y >= kInterludeExitMinY && centre.y <= kInterludeExitMaxY;
}

} // namespace nccu

#endif // INTERLUDE_EXIT_H_
