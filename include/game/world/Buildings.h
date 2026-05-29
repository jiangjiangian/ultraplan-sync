#ifndef BUILDINGS_H_
#define BUILDINGS_H_
#include "engine/math/Rect.h"
#include <array>
#include <string_view>

/**
 * @file Buildings.h
 * @brief 政大山下校園 26 棟建築的靜態資料表：名稱、觸發矩形與鏡像旗標，
 *        由 Tiled 匯出工具自動產生。
 */

namespace nccu::buildings {

/**
 * @brief 單棟建築的純資料記錄：名稱、進入觸發矩形與貼圖鏡像旗標。
 */
struct Building {
    std::string_view name;            ///< 建築顯示名稱（繁中）
    nccu::engine::math::Rect  triggerRect; ///< 進入觸發區（BuildingTracker 以此判定章節進入事件）
    bool             flipX = false;   ///< 對應 Tiled 的水平翻轉位；預設 false 讓舊式兩欄初始化仍可編譯
    bool             flipY = false;   ///< 對應 Tiled 的垂直翻轉位
};

/**
 * @brief 政大山下 26 棟建築的觸發資料表，由 tiled_to_world.py 自動匯出。
 *
 * width/height 為實際擺放的 sprite 矩形，flipX/flipY 為 Tiled 鏡像狀態。此矩形
 * 僅作為 BuildingTracker 的進入觸發區；實體碰撞形狀另由像素級可走遮罩
 * (CollisionMask) 描述，與此表分離。於 Tiled 重新定位後重跑工具並貼回下方
 * 區塊即可。羅馬廣場無進入項——其開放廣場直接烘焙進底圖。
 */
inline constexpr std::array<Building, 26> kAll = {{
    {"大勇樓", { 1776.0f,  1021.0f,  252.0f,  211.0f}, false, false},
    {"大仁樓", { 1808.0f,  1199.0f,  242.0f,  206.0f}, false, false},
    {"大智樓", { 1808.0f,  1363.0f,  250.0f,  249.0f}, false, false},
    {"學思樓", {  320.0f,  1000.0f,  276.0f,  162.0f}, false, false},
    {"商學院", {  464.0f,   992.0f,  318.0f,  310.0f}, true, false},
    {"四維堂", { 1048.0f,   727.0f,  146.0f,  126.0f}, false, false},
    {"果夫樓", { 1069.0f,  1194.0f,  202.0f,  162.0f}, false, false},
    {"志希樓", {  811.0f,  1177.0f,  153.0f,  154.0f}, true, false},
    {"校友服務中心", { 1114.0f,  1421.0f,  244.0f,  219.0f}, false, false},
    {"樂活館", { 1293.0f,  1550.0f,  178.0f,  156.0f}, false, false},
    {"樂活小舖", { 1388.0f,  1578.0f,  309.0f,  205.0f}, false, false},
    {"游泳館", {  313.0f,   328.0f,  366.0f,  199.0f}, false, false},
    {"綜合院館", { 1681.0f,   677.0f,  371.0f,  326.0f}, false, false},
    {"法學院", {  441.0f,   653.0f,  271.0f,  336.0f}, false, false},
    {"研究大樓", {  590.0f,  1453.0f,  367.0f,  255.0f}, true, false},
    {"井塘樓", {   27.0f,   604.0f,  321.0f,  257.0f}, true, false},
    {"新聞館", { 1580.0f,  1301.0f,  159.0f,  109.0f}, true, false},
    {"集英樓", { 1524.0f,  1353.0f,  224.0f,  192.0f}, false, false},
    {"資訊大樓", { 1334.0f,  1050.0f,  319.0f,  233.0f}, false, false},
    {"風雩樓", { 1091.0f,   429.0f,  178.0f,  121.0f}, false, false},
    {"風雩走廊", { 1242.0f,     8.0f,  154.0f,  108.0f}, true, false},
    {"行政大樓", { 1057.0f,   598.0f,  240.0f,  156.0f}, true, false},
    {"體育館", { 1493.0f,   306.0f,  418.0f,  235.0f}, true, false},
    {"操場", { 1384.0f,   541.0f,  621.0f,  399.0f}, false, false},
    {"正門", {  887.0f,  1722.0f,  101.0f,  116.0f}, false, false},
    {"中正圖書館", {  698.0f,   254.0f,  382.0f,  255.0f}, false, false},
}};

} // namespace nccu::buildings

#endif // BUILDINGS_H_
