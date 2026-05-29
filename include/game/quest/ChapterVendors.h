#ifndef CHAPTER_VENDORS_H_
#define CHAPTER_VENDORS_H_
#include "game/vendor/VendorConfig.h"
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"
#include <string>
#include <vector>

namespace nccu {

/**
 * @file ChapterVendors.h
 * @brief 各章節的攤販名冊，ChapterNpcSpawns 的價目表姊妹。
 *
 * Vendor「不是」NpcSpawn（它需要 VendorConfig，而非 sprite 路徑＋npcId），故另設
 * 自己的配置表。World 在 RespawnChapterRoster 內與 ChapterNpcSpawns 一併迭代它。
 *
 * Interlude 陣容不再是手寫字面值——它在執行期由 LoadInterludeVendors 從
 * interlude_market.md 解析（.md 是單一事實來源，與章節對話完全相同），再與程式端
 * 的位置表 zip 起來（空間排版是程式的工作，與 NpcSpawns 同）。結果有快取（首次呼
 * 叫即解析；參考在 ReloadVendors() 前皆有效）。其他狀態回傳空的靜態向量——各章的
 * 附帶攤販於後續加入。
 */
struct VendorPlacement {
    VendorConfig    config;          ///< 攤位設定
    nccu::engine::math::Vec2 pos;    ///< 攤位世界座標
};

/**
 * @brief 取得指定章節狀態的攤販配置（快取）。
 * @param state 學期章節狀態。
 * @return 該狀態的攤販配置向量；Interlude 由解析器供應，其餘目前多為空。
 */
const std::vector<VendorPlacement>& ChapterVendors(SemesterState state);

/**
 * @brief 設定 LoadInterludeVendors 讀取的內容目錄。
 * @param dir 內容目錄（預設 "docs/content"，對應 dialog::SetContentDir）。
 *
 * 測試會把它指向測試內容目錄。變更它會使快取失效。
 */
void SetVendorContentDir(std::string dir);

/**
 * @brief 丟棄解析快取，使下次 ChapterVendors() 重新讀取 .md（熱重載）。
 *
 * 為 dialog::Reload() 的姊妹。
 */
void ReloadVendors();

} // namespace nccu

#endif // CHAPTER_VENDORS_H_
