#ifndef CHAPTER_QUEST_ITEMS_H_
#define CHAPTER_QUEST_ITEMS_H_
#include "game/quest/Chapter1Quest.h"
#include "game/quest/Chapter2Quest.h"
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"
#include <string>
#include <vector>

namespace nccu {

/**
 * @file ChapterQuestItems.h
 * @brief 各章節的任務物品配置——ChapterPickups（現金）／ChapterVendors（攤位）／
 *        ChapterNpcSpawns 的敘事姊妹。
 *
 * World 在 SpawnChapterNpcs 把它們生成為 QuestFlagPickup 並納入章節名冊，故未拾取
 * 的筆記會在下次狀態變更時清掃（筆記絕不漏出其所屬章節）。Ch1 的申請書「不」在此
 * ——它仍由建構式生成（一個永久的 Ch1 物件，自有先例）；本表用於章節範圍、由名冊
 * 清掃的任務物品。
 *
 * Ch1 = 苦主的透明傘（善有善報重新設計）：西裝學長掉在集英樓附近的可尋物品，撿起
 * 設 Flag_HasVictimUmbrella，再「帶回」交給苦主（真正的章節結束是
 * TryReturnVictimUmbrella 的授予，「非」此拾取——撿地上的傘不會結束 Ch1）。它在進
 * 入章節時透過共用的 SpawnChapterQuestItems helper 生成（不同於 Ch2 筆記要等學霸被
 * 喚醒才延後生成）。Ch2 = 三頁散落筆記；收齊三頁者透過 QuestFlagPickup 的完成鉤子
 * 賺得學霸 (b) 的業力 +3，圖書館管理員 (b) 隨後指向羅馬廣場。
 */
struct QuestItemPlacement {
    nccu::engine::math::Vec2          pos;     ///< 任務物品世界座標
    std::string              flag;             ///< 拾取後設置的旗標
    std::string              message;          ///< 拾取時的 ShowMessage
    std::vector<std::string> completionFlags;  ///< 全部設置即授予業力
    int                      completionKarma;  ///< 完成時授予的業力
    /// 選用的「依數量」台詞：非空時，拾取訊息依玩家此刻持有的 completionFlags
    /// 「數量」（第 1／2／3 個收到）挑選，而非依物品本身。讓三頁筆記在「任何」拾
    /// 取順序下都讀作「第一／第二／最後」。為空時改用單一 message。
    std::vector<std::string> countMessages = {};
};

/**
 * @brief 取得指定章節狀態的任務物品配置。
 * @param state 學期章節狀態。
 * @return 該狀態的任務物品向量（無任務物品的狀態回傳空向量）。
 */
inline const std::vector<QuestItemPlacement>&
ChapterQuestItems(SemesterState state) {
    // 三頁筆記共用同一組完成集合與業力；「最後」被撿到的那頁會看見每個旗標皆已設
    // 置，恰好授予學霸 (b) +3 一次（QuestFlagPickup::OnPickup——較早的同伴會看見缺
    // 少的旗標而跳過，已撿的同伴則已停用）。
    //
    // 它們延後生成（World::MaybeSpawnChapter2Notes），故「僅」在玩家喚醒學霸並由他
    // 提出請求「之後」才出現，絕不在進入章節時出現。座標散布於三個不同校園區域，
    // 使收集值得到處走（而非聚在圖書館一帶）：note1 在西北法學院附近、note2 在東南
    // 集英樓／新聞館附近、note3 在南側校友服務中心／正門附近。全部已遮罩驗證為嚴格
    // 可走「且」能自廣場／學霸 flood 抵達（完整的學霸→筆記→筆記→筆記→學霸迴圈已驗
    // 證；較早選在東側 (1480,1120) 的點雖可走卻被牆封住、不可達，故移至此處）。
    //
    // 訊息採「依數量」（kNoteMsgs），依玩家此刻持有幾頁挑選——第一／第二／最後——
    // 而非依哪一頁。故先撿 note3 也會正確印出「第一張」台詞，修掉舊的以身分為鍵之
    // 錯誤（先撿 note3 會錯誤宣告「最後一頁」）。message 仍保留為合理退路，但在提
    // 供 kNoteMsgs 時不使用。
    static const std::vector<std::string> kNoteSet = {
        kFlagFoundNote1, kFlagFoundNote2, kFlagFoundNote3};
    static const std::vector<std::string> kNoteMsgs = {
        "撿到一頁學霸的筆記。還有兩頁散在別處。",
        "第二頁筆記到手——空白處寫著「從現在開始」。",
        "最後一頁找齊了。三頁都在手上了，回去找學霸。",
    };
    static const std::vector<QuestItemPlacement> kChapter2 = {
        {{ 450.0f,  850.0f}, kFlagFoundNote1,
         "撿到一頁學霸的筆記。", kNoteSet, 3, kNoteMsgs},
        {{1400.0f, 1250.0f}, kFlagFoundNote2,
         "撿到一頁學霸的筆記。", kNoteSet, 3, kNoteMsgs},
        {{1040.0f, 1640.0f}, kFlagFoundNote3,
         "撿到一頁學霸的筆記。", kNoteSet, 3, kNoteMsgs},
    };
    // Ch1 = 苦主的透明傘。單一旗標、無完成集合（業力 0——+5 善行落在 (b) 承諾選項
    // 上，而授予本身即回報，並非此拾取）。置於 (1700,1610)，恰在集英樓正南、距西裝
    // 學長 (1620,1560) 約 94 px：學長「拿著透明傘往集英樓方向跑」，故它讀作他掉在
    // 那裡的傘。選此點是因可走淨空很寬（約 96 px 的開闊方塊），使測試的軸向驅動器
    // 能經東側淨空走廊（x≈1744-1752）穩健地走到，避開集英樓西側那些會讓純曼哈頓尋
    // 路死鎖的狹窄斜向縫隙。已遮罩驗證為嚴格可走「且」能自綜合院館的苦主 flood 抵
    // 達。
    static const std::vector<QuestItemPlacement> kChapter1 = {
        {{1700.0f, 1610.0f}, kFlagHasVictimUmbrella,
         "撿到一把眼熟的透明傘，傘柄上有 A 君的名字貼紙。", {}, 0, {}},
    };
    static const std::vector<QuestItemPlacement> kNone;

    switch (state) {
        case SemesterState::Chapter1_AddDrop:   return kChapter1;
        case SemesterState::Chapter2_Midterms:  return kChapter2;
        case SemesterState::Interlude_Market:
        case SemesterState::Chapter3_SportsDay:
        case SemesterState::Chapter4_Finals:
        case SemesterState::Ending_A:
        case SemesterState::Ending_B:
        case SemesterState::Ending_D:
        case SemesterState::Ending_C:
            return kNone;
    }
    return kNone;  // 不可達；使非 void 路徑完整
}

} // namespace nccu

#endif // CHAPTER_QUEST_ITEMS_H_
