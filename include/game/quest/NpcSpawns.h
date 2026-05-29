#ifndef NPC_SPAWNS_H_
#define NPC_SPAWNS_H_
#include "engine/math/Vec2.h"
#include <string>
#include <string_view>
#include <vector>

namespace nccu {

/**
 * @file NpcSpawns.h
 * @brief 設計文件中的 NPC 生成名冊（Ch1 預設、環境路人、Ch1 群眾與風味 NPC）。
 *
 * 每個原型停在其錨點建築觸發矩形的南緣，使其讀作站在入口處。sprite 路徑指向
 * resources/assets/sprites/ 下精選的 Pipoya 子集。每個 NPC 都坐在其錨點建築碰撞矩
 * 形「之外」（觸發矩形由 main.cpp 的 kBuildingInset 內縮），使走近對話的玩家不會
 * 被牆推開。
 */
struct NpcSpawn {
    nccu::engine::math::Vec2   pos;   ///< 生成世界座標
    const char* spritePath;           ///< sprite 圖檔路徑
    const char* npcId;                ///< 對話內容鍵；"" = 無對話（環境路人）
    bool        isQuestGiver;         ///< 是否任務給予者（畫「!」）
    bool        wander = false;       ///< 是否啟用隨機遊走
};

/**
 * @brief Ch1 預設 NPC 名冊（5 個設計原型）。
 * @return Ch1 的 NPC 生成向量。
 *
 * npcId 作為執行期對話內容的鍵——開場在對話時拉取對應 (npcId, SemesterState) 的台
 * 詞，故對話不再在此寫死。
 */
inline const std::vector<NpcSpawn>& DefaultNpcSpawns() {
    static const std::vector<NpcSpawn> kAll = {
        // 苦主——Ch1 任務給予者，置於綜合院館，善有善報重新設計從此開場：玩家自己
        // 的透明傘從綜院 1 樓傘架消失，而同樣在那裡丟了傘的苦主，就在建築西南角外
        // 被找到。(1660,1010) 已遮罩驗證為嚴格可走（自身與四鄰皆 100%、不在任何建
        // 築觸發內），並能自 (500,1860) 出生點經缺口列 flood 抵達。任務給予者「!」
        // 標記他為第一站。
        {nccu::engine::math::Vec2{1660, 1010}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "victim", true},
        // 西裝學長——在集英樓，即苦主所說他「拿著透明傘往集英樓方向跑」的目標建
        // 築。(1620,1560) 恰在集英樓矩形正南，已遮罩驗證為嚴格可走（自身與四鄰皆
        // 100%）並 flood 可達；苦主傘的拾取點就在附近 (1450,1450)。
        {nccu::engine::math::Vec2{1620, 1560}, "resources/assets/sprites/npc/suit_senior.png",
         "suit_senior", false},
        // 學霸——在中正圖書館前，對應 chapter1.md「場景：中正圖書館 1 樓入口附近，
        // 抱著一疊厚厚的講義」。(820,545) 恰在圖書館矩形正南（底 y=509）的入口前
        // 埕，已遮罩驗證為嚴格可走（自身與四鄰皆 100%）並能自 (500,1860) 出生點
        // flood 抵達。
        {nccu::engine::math::Vec2{820, 545}, "resources/assets/sprites/school_uniform_3/female_03.png",
         "bookworm", false},
        // 助教——在行政大樓前，對應 chapter1.md「場景：行政大樓 1 樓走廊，手持一疊
        // 加退選資料夾」。(1220,775) 恰在矩形正南（底 y=754）、中央校園東側的開闊前
        // 埕，已遮罩驗證為嚴格可走（自身與四鄰皆 100%）並 flood 可達。
        {nccu::engine::math::Vec2{1220, 775}, "resources/assets/sprites/npc/ta.png",
         "ta", false},
        // 福利社阿姨——在樂活小舖，對應 chapter1.md「場景：樂活小舖內，正在整理飲料
        // 架」。(1560,1560) 恰在建築北牆（y=1578）以北、對齊店面正中，已遮罩驗證為
        // 嚴格可走（自身與四鄰皆 100%）並 flood 可達；它與 Ch1 主線路徑的東側走廊保
        // 持 122 px 淨空，故可達性主軸不受影響。
        {nccu::engine::math::Vec2{1560, 1560}, "resources/assets/sprites/npc/shop_auntie.png",
         "shop_auntie", false},
    };
    return kAll;
}

/**
 * @brief 環境路人名冊——沿指南路與中央地帶飄移的學生，讓山下校園顯得有人氣。
 * @return 環境路人的生成向量。
 *
 * 無對話（玩家直接走過）、不阻擋、wander=true 使 World 串接 EnableWander()。生成
 * 點落在開闊道路、避開建築足跡。
 */
inline const std::vector<NpcSpawn>& AmbientStudentSpawns() {
    static const std::vector<NpcSpawn> kAll = {
        {nccu::engine::math::Vec2{ 700, 1880}, "resources/assets/sprites/school_uniform_3/male_01.png",   "", false, true},
        {nccu::engine::math::Vec2{1080, 1870}, "resources/assets/sprites/school_uniform_3/female_01.png", "", false, true},
        {nccu::engine::math::Vec2{1500, 1880}, "resources/assets/sprites/school_uniform_3/male_03.png",   "", false, true},
        // female_02 / male_02 原本嵌在商學院／資訊大樓的牆基（遊戲中看起來像「有人
        // 卡在牆裡」）；已移到開闊的中央校園，並驗證可走＋可達。
        {nccu::engine::math::Vec2{ 980, 1640}, "resources/assets/sprites/school_uniform_3/female_02.png", "", false, true},
        {nccu::engine::math::Vec2{1450, 1620}, "resources/assets/sprites/school_uniform_3/male_02.png",   "", false, true},
        {nccu::engine::math::Vec2{ 980, 1500}, "resources/assets/sprites/school_uniform_3/female_03.png", "", false, true},
    };
    return kAll;
}

/**
 * @brief Ch1 加退選搶課群眾名冊——一批散布於中央校園的遊走學生，傳達搶課的擁擠感。
 * @return Ch1 群眾的生成向量。
 *
 * 每筆與 AmbientStudentSpawns 完全相同：無 npcId（無對話、無「!」）、不阻擋
 * （wander_ 時 NPC::BlocksMovement 為 false），故會隨機走動並播動畫，玩家直接穿過
 * 它們。sprite 沿用已快取的 Pipoya 校服子集（便宜——無新貼圖）。全部座標已遮罩驗
 * 證為嚴格可走（自身與四鄰皆 100%）並能自 (500,1860) 出生點 flood 抵達，且「全
 * 部」坐在中央校園（y≈860..1180）——遠離 Ch1 苦主／學長／助教／阿姨／學霸的任務錨
 * 點與南側指南路走廊（冒煙測試腳本行經之處），故既不擋任務互動、也不擾動不與它們
 * 對話的腳本。World::SpawnChapterNpcs 為每筆串接 EnableWander ＋ SetWanderMask（僅
 * Ch1；離開章節時由章節名冊清掃，與 Ch3 操場群眾相同）。
 */
inline const std::vector<NpcSpawn>& Chapter1CrowdSpawns() {
    static const std::vector<NpcSpawn> kAll = {
        {nccu::engine::math::Vec2{1000,  900}, "resources/assets/sprites/school_uniform_3/male_04.png",   "", false, true},
        {nccu::engine::math::Vec2{ 900,  950}, "resources/assets/sprites/school_uniform_3/female_04.png", "", false, true},
        {nccu::engine::math::Vec2{ 960, 1080}, "resources/assets/sprites/school_uniform_3/male_05.png",   "", false, true},
        {nccu::engine::math::Vec2{1140, 1140}, "resources/assets/sprites/school_uniform_3/female_05.png", "", false, true},
        {nccu::engine::math::Vec2{ 840,  860}, "resources/assets/sprites/school_uniform_3/male_07.png",   "", false, true},
        {nccu::engine::math::Vec2{1240,  980}, "resources/assets/sprites/school_uniform_3/female_06.png", "", false, true},
        {nccu::engine::math::Vec2{1000, 1180}, "resources/assets/sprites/school_uniform_3/male_08.png",   "", false, true},
        {nccu::engine::math::Vec2{ 880, 1120}, "resources/assets/sprites/school_uniform_3/female_07.png", "", false, true},
    };
    return kAll;
}

/**
 * @brief Ch1 固定式風味 NPC 名冊——幾個站著抱怨搶課／閒聊的學生（按 E 互動）。
 * @return Ch1 風味 NPC 的生成向量。
 *
 * 固定（wander=false → 實心，玩家會撞開）且非任務（isQuestGiver=false → 無「!」，
 * Ch1IndicatorVisible 的退路回傳其 false 位）。每筆帶有不同的 npcId，其 chapter1.md
 * 「## NPC：…」(a) 段落即為它的台詞池。World::SpawnChapterNpcs 在生成時透過
 * NPC::LoadDialog 把該池載入 NPC 的對話列；GameController 的 E 互動把風味 npcId 導
 * 向 NPC::Interact()，每次對話循環取池中一句（索引以池大小取模前進）——是「確定
 * 性、可重現」的「隨機」挑選（無 std::rand／時間種子；測試維持 byte 一致）。風味
 * 路徑「絕不」執行主線鉤子，也「絕不」設任務旗標，故硬性把守的 苦主→學長→苦主 主
 * 線不受影響。座標落在中央校園、距每個任務錨點 ≥90 px、遠離冒煙測試走廊（使阻擋
 * 的風味 NPC 無法偏移被腳本操控的玩家）。已遮罩驗證可走＋ flood 可達；離開章節時隨
 * 其餘 Ch1 名冊一併清掃。
 */
inline const std::vector<NpcSpawn>& Chapter1FlavorSpawns() {
    static const std::vector<NpcSpawn> kAll = {
        {nccu::engine::math::Vec2{1010, 1100}, "resources/assets/sprites/school_uniform_3/male_09.png",   "ch1_flavor_grab", false, false},
        {nccu::engine::math::Vec2{ 900, 1000}, "resources/assets/sprites/school_uniform_3/female_08.png", "ch1_flavor_rain", false, false},
        {nccu::engine::math::Vec2{1150, 1050}, "resources/assets/sprites/school_uniform_3/male_11.png",   "ch1_flavor_bag",  false, false},
    };
    return kAll;
}

/**
 * @brief 此 npcId 是否為 Ch1 的固定式風味 NPC 之一。
 * @param npcId 要查詢的 NPC 識別字串。
 * @return 是回傳 true；空／未知 id 回傳 false（任務／環境 NPC 不受影響）。
 *
 * 單一事實來源（由 Chapter1FlavorSpawns 推導，使集合永不漂移）。GameController 的
 * E 互動分派以此把風味 NPC 導向循環的 NPC::Interact() 路徑，「而非」主線鉤子＋
 * DialogOpener——使「不設任何任務旗標」的保證成為結構性（風味 NPC 根本不會抵達任
 * 務鉤子）。
 */
[[nodiscard]] inline bool IsChapter1FlavorNpc(std::string_view npcId) noexcept {
    if (npcId.empty()) return false;
    for (const auto& s : Chapter1FlavorSpawns())
        if (npcId == s.npcId) return true;
    return false;
}

} // namespace nccu

#endif // NPC_SPAWNS_H_
