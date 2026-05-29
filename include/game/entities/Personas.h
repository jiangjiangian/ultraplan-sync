#ifndef GAME_ENTITIES_PERSONAS_H_
#define GAME_ENTITIES_PERSONAS_H_
#include "engine/math/Color.h"
#include <array>
#include <string>
#include <string_view>

/**
 * @file Personas.h
 * @brief 角色人設的領域資料：可選人設表與每場選擇結果，屬 Model 而非 UI 呈現。
 */

namespace nccu {

// 角色人設的領域資料，自 ui/CharacterSelect.h 移出。人設定義（label／blurb／
// spritePath／tint）與每場選擇結果屬 MODEL 資料而非 UI 呈現——資產預熱
// （TexturePreload）、GameplayScene 套用人設色調、以及自動播放略過路徑皆會取用它們。
// ui/CharacterSelect.h 仍透過傳遞性 include 再匯出它們以維持原始碼相容。

/**
 * @brief 一個可選的角色人設。
 *
 * 非二元的校園原型（政大山下學生人設），而非男／女的二選一。每個都對應到
 * resources/assets/sprites/school_uniform_3/ 之下既有的 Pipoya 圖集（不新增任何二進位
 * 檔到 resources/）；恰好共用同一張底圖的不同人設，於繪製時以執行期的 DrawTexturePro
 * 色彩 tint（raylib 5.5 colour-modulate）拉開差異，故五個都讀來視覺有別而無須任何新美術。
 * tint 也會蓋印到地圖內的 Player sprite（View 以所選人設的顏色為前景物件上色）。
 */
struct Persona {
    std::string_view label;     ///< 選單顯示的 CJK 人設名稱
    std::string_view blurb;     ///< 名稱下方的一行風味說明
    std::string_view spritePath;///< 資源相對路徑的 Pipoya 圖集
    nccu::engine::math::Color        tint;      ///< 繪製時的色彩調變（RGBA）
};

// 五個人設。順序即選單順序（索引 0..4）。固定於編譯期，使自動播放可不執行選擇器、直接由
// UMBRELLA_SPRITE 決定性地解析出 sprite 路徑／色調。
inline constexpr std::array<Persona, 5> kPersonas{{
    {"夜貓子", "通宵 K 書，黑眼圈是勳章",
     "resources/assets/sprites/school_uniform_3/female_03.png",
     nccu::engine::math::Color{150, 170, 255, 255}},   // 冷調靛藍
    {"social咖", "系上活動的開心果",
     "resources/assets/sprites/school_uniform_3/male_02.png",
     nccu::engine::math::Color{255, 175, 90, 255}},    // 暖橙
    {"邊緣人", "圖書館角落的常駐住民",
     "resources/assets/sprites/school_uniform_3/female_01.png",
     nccu::engine::math::Color{170, 220, 180, 255}},   // 柔和綠
    {"卷王", "GPA 4.3，行事曆排到深夜",
     "resources/assets/sprites/school_uniform_3/male_03.png",
     nccu::engine::math::Color{255, 150, 170, 255}},   // 玫瑰紅
    {"佛系生", "隨緣修課，傘丟了也不急",
     "resources/assets/sprites/school_uniform_3/female_02.png",
     nccu::engine::math::Color{210, 200, 120, 255}},   // 琥珀
}};

/**
 * @brief 每場的人設選擇結果——CharacterSelectScene 確認、GameplayScene 取用之值。
 *
 * 自動播放略過路徑會直接由 UMBRELLA_SPRITE 建構一個。
 */
struct CharacterSelectResult {
    std::string spritePath;                            ///< 所選人設的 sprite 路徑
    nccu::engine::math::Color  tint{255, 255, 255, 255};  ///< 所選人設的色調（預設白＝不調色）
    bool        closed{false};                         ///< 選擇器是否被關閉／取消
};

} // namespace nccu

#endif // GAME_ENTITIES_PERSONAS_H_
