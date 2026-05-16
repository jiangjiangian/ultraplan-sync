#ifndef BUILDINGS_H_
#define BUILDINGS_H_
#include "gfx/Rect.h"
#include <array>
#include <string_view>

namespace nccu::buildings {

struct Building {
    std::string_view name;
    nccu::gfx::Rect  triggerRect;
    // Mirror flags emitted by tools/tiled_to_world.py from the Tiled
    // flip bits. Defaulted so any legacy 2-field initializer still
    // compiles; the regenerated block carries the real flip state.
    bool             flipX = false;
    bool             flipY = false;
};

// 26 NCCU 山下 buildings. Auto-emitted by tools/tiled_to_world.py:
// width/height are the placed sprite rect, flipX/flipY the Tiled mirror
// state. This rect is the trigger zone BuildingTracker keys on (chapter
// entry events); the physical collision shape is authored separately in
// Tiled and lives in Obstacles.h colliders::kAll. Re-run the tool after
// re-positioning in Tiled and paste the printed block below. 羅馬廣場
// has no entry — its open plaza is baked into the base map.
inline constexpr std::array<Building, 26> kAll = {{
    {"大勇樓", { 1776.0f,  1021.0f,  252.0f,  211.0f}, false, false},
    {"大仁樓", { 1808.0f,  1199.0f,  242.0f,  206.0f}, false, false},
    {"大智樓", { 1808.0f,  1363.0f,  250.0f,  249.0f}, false, false},
    {"學思樓", {  333.0f,  1012.0f,  276.0f,  162.0f}, false, false},
    {"商學院", {  464.0f,   992.0f,  318.0f,  310.0f}, true, false},
    {"四維堂", { 1048.0f,   727.0f,  146.0f,  126.0f}, false, false},
    {"果夫樓", { 1069.0f,  1194.0f,  202.0f,  162.0f}, false, false},
    {"志希樓", {  811.0f,  1177.0f,  153.0f,  154.0f}, true, false},
    {"校友服務中心", { 1131.0f,  1419.0f,  244.0f,  219.0f}, false, false},
    {"樂活館", { 1290.0f,  1562.0f,  178.0f,  156.0f}, false, false},
    {"樂活小舖", { 1388.0f,  1594.0f,  309.0f,  205.0f}, false, false},
    {"游泳館", {  313.0f,   328.0f,  366.0f,  199.0f}, false, false},
    {"綜合院館", { 1681.0f,   677.0f,  371.0f,  326.0f}, false, false},
    {"法學院", {  441.0f,   667.0f,  271.0f,  336.0f}, false, false},
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
    {"中正圖書館", {  705.0f,   279.0f,  382.0f,  255.0f}, false, false},
}};

} // namespace nccu::buildings

#endif // BUILDINGS_H_
