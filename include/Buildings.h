#pragma once
#include "gfx/Rect.h"
#include <array>
#include <string_view>

namespace nccu::buildings {

struct Building {
    std::string_view name;
    nccu::gfx::Rect  triggerRect;
};

// 27 NCCU 山下 buildings. Coordinates auto-emitted from
// tools/composite_worldmap.py — width/height are the ACTUAL placed sprite
// dimensions after bbox-crop + fit-to-height. Earlier versions of this
// table used target_h for both axes and the trigger box covered only the
// central 60% of each visual building, letting the player walk on the
// overhanging roof. Run the python tool after re-positioning any building
// and paste the printed block below.
inline constexpr std::array<Building, 27> kAll = {{
    {"游泳館",       {  50.0f,  630.0f, 301.0f, 180.0f}},
    {"樂活館",       { 380.0f,  630.0f, 281.0f, 180.0f}},
    {"研究大樓",     { 672.0f,  640.0f, 296.0f, 160.0f}},
    {"中正圖書館",   { 198.0f,  890.0f, 324.0f, 200.0f}},
    {"操場",         {1206.0f,  670.0f, 348.0f, 200.0f}},
    {"體育館",       {1635.0f,  520.0f, 231.0f, 180.0f}},
    {"羅馬廣場",     { 896.0f,  980.0f, 209.0f, 200.0f}},
    {"綜合院館",     {1612.0f,  810.0f, 177.0f, 180.0f}},
    {"校友服務中心", {1740.0f, 1010.0f, 280.0f, 180.0f}},
    {"行政大樓",     {  69.0f, 1330.0f, 222.0f, 200.0f}},
    {"樂活小舖",     { 296.0f, 1340.0f, 289.0f, 180.0f}},
    {"商學院",       { 595.0f, 1340.0f, 250.0f, 180.0f}},
    {"新聞館",       { 848.0f, 1340.0f, 274.0f, 180.0f}},
    {"資訊大樓",     {1135.0f, 1340.0f, 291.0f, 180.0f}},
    {"風雩樓",       {1448.0f, 1340.0f, 304.0f, 180.0f}},
    {"大仁樓",       {1798.0f, 1360.0f, 244.0f, 140.0f}},
    {"法學院",       {  37.0f, 1520.0f, 146.0f, 200.0f}},
    {"井塘樓",       { 196.0f, 1530.0f, 269.0f, 180.0f}},
    {"四維堂",       { 473.0f, 1530.0f, 214.0f, 180.0f}},
    {"果夫樓",       { 724.0f, 1540.0f, 292.0f, 160.0f}},
    {"集英樓",       {1018.0f, 1530.0f, 245.0f, 180.0f}},
    {"大勇樓",       {1269.0f, 1530.0f, 322.0f, 180.0f}},
    {"學思樓",       {1608.0f, 1530.0f, 245.0f, 180.0f}},
    {"風雩走廊",     { 737.0f, 1720.0f, 326.0f, 100.0f}},
    {"志希樓",       {1218.0f, 1700.0f, 224.0f, 140.0f}},
    {"大智樓",       {1496.0f, 1700.0f, 248.0f, 140.0f}},
    {"正門",         {  26.0f, 1820.0f, 309.0f, 140.0f}},
}};

} // namespace nccu::buildings
