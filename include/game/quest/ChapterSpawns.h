#ifndef CHAPTER_SPAWNS_H_
#define CHAPTER_SPAWNS_H_
#include "game/quest/NpcSpawns.h"
#include "game/state/SemesterState.h"
#include <vector>

namespace nccu {

/**
 * @file ChapterSpawns.h
 * @brief 以 SemesterState 為鍵的章節 NPC 名冊。
 *
 * World::RespawnChapterRoster 在每次學期狀態機推進時讀取它，使玩家可對話的 NPC
 * 跟隨當前章節，而非凍結在建構式生成的 Ch1 那組。
 */

/**
 * @brief 取得指定章節狀態的 NPC 名冊。
 * @param state 學期章節狀態。
 * @return 該狀態的 NPC 生成向量。
 *
 * Ch1 委派給 DefaultNpcSpawns()，使名冊與現況 byte 完全相同（既有 Ch1 生成／可達
 * 性測試全綠，且不重複任何字面值）。環境路人「非」逐章：AmbientStudentSpawns() 維
 * 持全域，由建構式在地形遮罩載入後串接一次。
 */
inline const std::vector<NpcSpawn>& ChapterNpcSpawns(SemesterState state) {
    static const std::vector<NpcSpawn> kInterlude;     // 尚未配置章節名冊

    // Ch2 圖書館期中考。chapter2.md 有 6 個 NPC 段落；sprite 沿用 Ch1（美術之後再
    // 修）。位置依敘事地理（場景台詞）而非 Ch1 錨點：
    //   - 圖書館管理員（任務給予者）坐在中正圖書館服務台——恰在圖書館矩形正南（矩
    //     形底 y=509；服務台在 y=545）——因 chapter2.md 把整條主線都經她的櫃台。保
    //     留（Ch2 任務依賴她的位置）。
    //   - 學霸（bookworm）坐在羅馬廣場雕像下（廣場中心 ≈1088,960；置於南緣
    //     1088,1100）——各方向（note3／管理員(b)／旁白）都引導玩家去的救援節拍。保
    //     留（Ch2 救援節拍依賴它）。
    //   - 西裝學長 → 行政大樓正門前 (1220,775)：chapter2.md「場景：行政大樓正門附
    //     近，靠著柱子滑手機」。
    //   - 助教 → 中正圖書館前 (900,545)：chapter2.md「場景：中正圖書館…圖書館一樓
    //     走廊（背景 NPC 身份）」。
    //   - 苦主 → 中正圖書館前西側角 (720,560)：chapter2.md「場景：中正圖書館二樓閱
    //     覽區走廊角落」。坐在管理員櫃台西側，與共用圖書館前埕的助教／管理員／自動
    //     販賣機 (980,560) 群保持 >=80 px 淨空。
    //   - 福利社阿姨 → 樂活小舖 (1560,1560)：chapter2.md「場景：樂活小舖內」。
    // 4 個原型為選做／漣漪（isQuestGiver=false）。全部座標已遮罩驗證為嚴格可走＋
    // flood 可達，其 Ch2 期望檢查釘住管理員↔中正圖書館與學霸↔羅馬廣場。
    static const std::vector<NpcSpawn> kChapter2 = {
        {nccu::engine::math::Vec2{ 720,  560}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "victim", false},
        {nccu::engine::math::Vec2{1220,  775}, "resources/assets/sprites/npc/suit_senior.png",
         "suit_senior", false},
        {nccu::engine::math::Vec2{1088, 1100}, "resources/assets/sprites/school_uniform_3/female_03.png",
         "bookworm", false},
        {nccu::engine::math::Vec2{ 900,  545}, "resources/assets/sprites/npc/ta.png",
         "ta", false},
        {nccu::engine::math::Vec2{1560, 1560}, "resources/assets/sprites/npc/shop_auntie.png",
         "shop_auntie", false},
        {nccu::engine::math::Vec2{ 820,  545}, "resources/assets/sprites/school_uniform_3/female_01.png",
         "librarian", true},
    };
    // Ch3 校慶運動會。5 個原型（漣漪／選做，isQuestGiver=false）改依其場景台詞而
    // 非 Ch1/Ch2 錨點：
    //   - 西裝學長 → 操場看台邊緣 (1430,910)：chapter3.md「場景：操場看台邊緣，啦
    //     啦隊加油區旁」。
    //   - 助教 → 操場邊緣折疊桌 (1530,930)：chapter3.md「場景：操場邊緣的折疊桌…
    //     負責登記活動出席」。
    //   - 苦主 → 操場邊緣角落 (1620,920)：chapter3.md「場景：操場邊緣，不在人群裡
    //     …在角落擺了一個小販攤」。
    //   - 福利社阿姨 → 四維道 (1100,860)：chapter3.md「場景：四維道擺臨時攤」（恰
    //     在四維堂矩形正南）。
    //   - 學霸 → 中正圖書館 (900,545)：chapter3.md「場景：不在操場。在中正圖書館…
    //     繼續讀書」——他刻意跳過校慶，故在圖書館而「非」操場。
    // 3 個操場原型坐在場地南緣（y≈910-930），全在跑者圓圈外（跑道中心 1694,740、
    // r150——距中心 195-314 px），並與最近的閒置者保持 ≥128 px，故絕不坐在跑圈跑道
    // 上或與校慶人群重疊。
    // 3 個物物交換鏈任務給予者（A 系香腸／B 系大聲公／C 系學姊，isQuestGiver=true）
    // 仍散布於羅馬廣場——刻意避開中央校門→圖書館的通道以免擋路（玩家要求，刻意覆寫
    // 其敘事場景）——亦即玩家跑完操場後前往之處。它們的「!」指示燈「依序」揭露
    // （Ch3IndicatorVisible：A 完成交易前是 A，再 B、再 C）而非一次全亮，且鏈必須
    // 走 A→B→C（TryAdvanceCh3Trade 重導越序的對話）。全部座標已遮罩驗證為嚴格可走
    // ＋ flood 可達。
    static const std::vector<NpcSpawn> kChapter3 = {
        {nccu::engine::math::Vec2{1620,  920}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "victim", false},
        {nccu::engine::math::Vec2{1430,  910}, "resources/assets/sprites/npc/suit_senior.png",
         "suit_senior", false},
        {nccu::engine::math::Vec2{ 900,  545}, "resources/assets/sprites/school_uniform_3/female_03.png",
         "bookworm", false},
        {nccu::engine::math::Vec2{1530,  930}, "resources/assets/sprites/npc/ta.png",
         "ta", false},
        {nccu::engine::math::Vec2{1100,  860}, "resources/assets/sprites/npc/shop_auntie.png",
         "shop_auntie", false},
        {nccu::engine::math::Vec2{ 980, 1000}, "resources/assets/sprites/npc/shop_auntie.png",
         "vendor_sausage_a", true},
        {nccu::engine::math::Vec2{1150, 1010}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "loudspeaker_b", true},
        {nccu::engine::math::Vec2{1060, 1120}, "resources/assets/sprites/school_uniform_3/female_01.png",
         "senior_c", true},
    };
    // Ch4 期末考終焉。chapter4.md 有 5 個 ## NPC：段落——巔峰強度下的 5 個原型，無
    // 新 NPC。全 isQuestGiver=false（終盤是閘門驅動而非任務給予者驅動——Ending
    // A/B/C 在 CheckEndingGates 解析；助教雖居中，但經 (d) 體諒選項而非任務給予者標
    // 記）。sprite 沿用既有美術；位置沿用已驗證可走的座標（可達性如各處一樣是人工
    // 驗證項）。
    // 西裝學長仍出現在此列表中，因為這張表是純資料；Ch4「斥責後不出場」漣漪改由
    // World::SpawnChapterNpcs 內的生成期過濾器強制執行——當玩家持
    // Flag_ScoldedSenior 而未持彌補的 Flag_HelpedSenior（Ch2 回呼）時，跳過把
    // suit_senior 推入物件容器。把閘門放在生成器而非資料表，與「管理員只在 Ch2」的
    // 作法一致：一個「不在本章」的 NPC 等同建模為「不在物件容器中」，章節名冊拆除
    // 與 Chapter4Quest 分流兩者皆自然運作。
    //
    // 位置依 chapter4.md 場景台詞：
    //   - 西裝學長 → 行政大樓門口 (1220,775)：「場景：行政大樓門口
    //     (Flag_HelpedSenior = true 路徑)」。
    //   - 助教 → 研究大樓走廊 (980,1560)：「場景：研究大樓走廊中段」（恰在研究大樓
    //     矩形正東）。
    //   - 苦主 → 正門旁 (1010,1700)：「場景：正門旁的廊柱下」（正門矩形東北；坐在
    //     南牆缺口、可達）。
    //   - 福利社阿姨 → 樂活小舖 (1560,1560)：「場景：樂活小舖，期末考週備品」。
    //   - 學霸 → 中正圖書館考場外 (900,545)：「場景：中正圖書館考場外走廊」。
    // 全 isQuestGiver=false（終盤是閘門驅動）。全部座標已遮罩驗證為嚴格可走＋
    // flood 可達。
    static const std::vector<NpcSpawn> kChapter4 = {
        {nccu::engine::math::Vec2{1010, 1700}, "resources/assets/sprites/school_uniform_3/male_02.png",
         "victim", false},
        {nccu::engine::math::Vec2{1220,  775}, "resources/assets/sprites/npc/suit_senior.png",
         "suit_senior", false},
        {nccu::engine::math::Vec2{ 900,  545}, "resources/assets/sprites/school_uniform_3/female_03.png",
         "bookworm", false},
        {nccu::engine::math::Vec2{ 980, 1560}, "resources/assets/sprites/npc/ta.png",
         "ta", false},
        {nccu::engine::math::Vec2{1560, 1560}, "resources/assets/sprites/npc/shop_auntie.png",
         "shop_auntie", false},
    };
    static const std::vector<NpcSpawn> kEndingA;       // 結局無名冊
    static const std::vector<NpcSpawn> kEndingB;       // 結局無名冊
    static const std::vector<NpcSpawn> kEndingC;       // 結局無名冊

    switch (state) {
        case SemesterState::Chapter1_AddDrop:   return DefaultNpcSpawns();
        case SemesterState::Interlude_Market:   return kInterlude;
        case SemesterState::Chapter2_Midterms:  return kChapter2;
        case SemesterState::Chapter3_SportsDay: return kChapter3;
        case SemesterState::Chapter4_Finals:    return kChapter4;
        case SemesterState::Ending_A:           return kEndingA;
        case SemesterState::Ending_B:           return kEndingB;
        case SemesterState::Ending_D:           return kEndingA;  // 無名冊
        case SemesterState::Ending_C:           return kEndingC;
    }
    return kInterlude;  // 不可達；使非 void 路徑完整
}

} // namespace nccu

#endif // CHAPTER_SPAWNS_H_
