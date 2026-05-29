#ifndef GFX_DECORATIONS_H_
#define GFX_DECORATIONS_H_
#include "game/gfx/SpriteStrip.h"
#include "engine/math/Vec2.h"
#include "game/quest/Chapter3Quest.h"   // kSportsTrackCx/Cy — 操場 centre
#include "game/state/SemesterState.h"
#include <array>

/**
 * @file Decorations.h
 * @brief 已擺放的氛圍裝飾表——純裝飾、無玩法影響。
 *
 * 屬 View 端資料（非 GameObject、不在 World::Objects() 內）：View 於建構時載入各
 * strip texture，並由 render clock 繪製，故模擬與序列化輸出皆看不到它們（序列化
 * 的是 World::Objects()，本表不在其中——有無美術，整局位元一致）。每個裝飾僅在
 * 學期 FSM 處於其 `chapter` 時顯示，且 strip PNG 缺檔時完全不畫（空資源路徑）
 * ——與本專案其他所有資產一致。
 *
 * 兩者皆為第三方同人圖、於 CREDITS.md 標註來源；二進位檔由使用者自行管理、不得
 * 提交入庫。請把轉好的 strip 放到下方 stripPath。
 */

namespace nccu::game::gfx {

/**
 * @brief 羅馬廣場吉伊卡哇「雕像」的世界座標中心。
 *
 * 學霸（bookworm）NPC 駐守在廣場南緣（1088,1100）以便玩家走近對話。雕像若放在廣場
 * 幾何中心（1088,960），會在學霸正上方約 140px，螢幕上看起來像漂浮於他北方的另一
 * 個物件，而非他癱睡其下的雕像。故下移至 1088,1040（在他南緣崗位上方 60px），讓
 * 24px 的玩家 + 坐著的學霸 + 約 80px 的雕像讀作「他靠在吉伊卡哇紀念碑下休息」的
 * 一組畫面。仍純裝飾——絕非 GameObject、不碰撞、不入序列化。
 */
inline constexpr nccu::engine::math::Vec2 kRomaPlazaStatue{1088.0f, 1040.0f};

/**
 * @brief Ch3 貓的繪製中心。
 *
 * 操場為矩形 (1384,541,621x399)，但綜合院館 (1681,677,371x326) 覆蓋操場東半部；
 * 若貓放在跑道幾何中心（kSportsTrackCx=1694, kSportsTrackCy=740），會落在綜院的
 * 佔地內、被建築 sprite 蓋住而看不見。故西移到綜院左緣（1681）以西、明確開闊的
 * 西側場地（x1530，遮罩驗證為嚴格可行走且深入 1384–2005 場地內），並維持同一跑道
 * 列（y740），使其仍讀作操場上的一隻貓，且不再被綜院遮擋。
 */
inline constexpr nccu::engine::math::Vec2 kSportsCatPos{1530.0f, kSportsTrackCy};

/**
 * @brief 裝飾表；僅供 View 依索引取用，順序無意義。
 *
 * - chiikawa — Ch2 期中考（Chapter2_Midterms）：羅馬廣場中心一尊脈動的「雕像」，
 *   約 80px 以讀作廣場紀念碑（24px 的玩家立於其旁顯得小得多）；乒乓縮放給出溫和的
 *   放大縮小呼吸感。
 * - cat — Ch3 校慶（Chapter3_SportsDay）：操場中心（kSportsTrackCx/Cy，即量測跑圈
 *   進度環所用的同一場地中心）一隻「小小的一隻、不要太大隻」的貓（約 28px），以同一
 *   套乒乓呼吸。
 */
inline constexpr std::array<DecorationDef, 2> kDecorations{{
    DecorationDef{SemesterState::Chapter2_Midterms, kRomaPlazaStatue,
                  "resources/assets/decorations/chiikawa_strip.png",
                  /*frameCount=*/17, /*drawScale=*/80.0f, /*fps=*/6.0},
    DecorationDef{SemesterState::Chapter3_SportsDay, kSportsCatPos,
                  "resources/assets/decorations/cat_strip.png",
                  /*frameCount=*/24, /*drawScale=*/28.0f, /*fps=*/8.0},
}};

} // namespace nccu::game::gfx

#endif // GFX_DECORATIONS_H_
