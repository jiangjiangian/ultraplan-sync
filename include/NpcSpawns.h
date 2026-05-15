#ifndef NPC_SPAWNS_H_
#define NPC_SPAWNS_H_
#include "gfx/Vec2.h"
#include <string>
#include <vector>

namespace nccu {

// The 5 NPC archetypes from the design doc, each parked at the south
// edge of their anchor building's trigger rect so they read as standing
// at the entrance. Sprite paths point at the curated Pipoya subset
// under resources/assets/sprites/ (see sprites/ATTRIBUTIONS.md). Each
// NPC sits just OUTSIDE its anchor building's collision rect (the
// trigger-rect shrunk by main.cpp's kBuildingInset) so a player walking
// up to talk does not get pushed away by the wall.
struct NpcSpawn {
    gfx::Vec2                pos;
    const char*              spritePath;
    std::vector<std::string> dialog;
    bool                     isQuestGiver;
    bool                     wander = false;
};

inline const std::vector<NpcSpawn>& DefaultNpcSpawns() {
    static const std::vector<NpcSpawn> kAll = {
        // 苦主 — quest-giver, on Zhinan Rd east of the 正門 gate footprint.
        {gfx::Vec2{380, 1860}, "resources/assets/sprites/school_uniform_3/male_02.png",
         {"我的傘…不見了…",
          "明明剛剛還放在傘架上的。",
          "如果你撿到別人的傘，記得物歸原主。"},
         true},
        // 西裝學長 — stern senior just north of 行政大樓 entrance.
        {gfx::Vec2{240, 1320}, "resources/assets/sprites/npc/suit_senior.png",
         {"學弟，山下校園的雨季比山上更難熬。",
          "撐傘的姿勢，決定別人怎麼看你。",
          "別把傘當成累贅，那是你身為大學生最後的體面。"},
         false},
        // 學霸 — south of 中正圖書館, between library and the gym row.
        {gfx::Vec2{560, 1280}, "resources/assets/sprites/school_uniform_3/female_03.png",
         {"下次小考的範圍我已經整理好了。",
          "圖書館 G 層的座位最安靜，記得避開窗邊。",
          "下雨天背書效率最高，這是科學。"},
         false},
        // 助教 — south of 學思樓, towards Zhinan Rd.
        {gfx::Vec2{1730, 1790}, "resources/assets/sprites/npc/ta.png",
         {"助教不是萬能，但你的程式還是得交。",
          "Office Hour 是我最後的救贖。",
          "下個禮拜小考，先把書讀完再來聊雨。"},
         false},
        // 福利社阿姨 — outside 樂活小舖, inside its trigger but clear of the wall.
        {gfx::Vec2{460, 1500}, "resources/assets/sprites/npc/shop_auntie.png",
         {"歡迎光臨！今天的茶葉蛋還熱著呢。",
          "孩子，傘要記得帶啊，淋濕了會感冒的。",
          "缺什麼來阿姨這裡看看，雜貨都有。"},
         false},
    };
    return kAll;
}

// Ambient pedestrians — students drifting along Zhinan Rd and the
// central strip to make the山下 campus feel populated. No dialog (the
// player walks straight past), non-blocking, wander=true so World wires
// EnableWander(). Spawn points sit on open road clear of footprints.
inline const std::vector<NpcSpawn>& AmbientStudentSpawns() {
    static const std::vector<NpcSpawn> kAll = {
        {gfx::Vec2{ 700, 1880}, "resources/assets/sprites/school_uniform_3/male_01.png",   {}, false, true},
        {gfx::Vec2{1080, 1870}, "resources/assets/sprites/school_uniform_3/female_01.png", {}, false, true},
        {gfx::Vec2{1500, 1880}, "resources/assets/sprites/school_uniform_3/male_03.png",   {}, false, true},
        {gfx::Vec2{ 640, 1180}, "resources/assets/sprites/school_uniform_3/female_02.png", {}, false, true},
        {gfx::Vec2{1340, 1180}, "resources/assets/sprites/school_uniform_3/male_02.png",   {}, false, true},
        {gfx::Vec2{ 980, 1500}, "resources/assets/sprites/school_uniform_3/female_03.png", {}, false, true},
    };
    return kAll;
}

} // namespace nccu

#endif // NPC_SPAWNS_H_
