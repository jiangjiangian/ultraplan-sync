#ifndef CHAPTER_PICKUPS_H_
#define CHAPTER_PICKUPS_H_
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"
#include <vector>

namespace nccu {

/**
 * @file ChapterPickups.h
 * @brief 各章節的零錢拾取點配置——迴圈經濟中「探索」收益來源。
 *
 * 為 ChapterNpcSpawns / ChapterVendors 的姊妹。掃地圖撿散落零錢的玩家藉此籌措下
 * 一個市集的花費；「德行」收益來源是業力把守的 NPC 打賞（各章），「策略」收益來
 * 源則是市集本身（募款箱／二手書）。
 *
 * World 在 SpawnChapterNpcs 生成它們並納入章節名冊，故未拾取的硬幣會在下次狀態變
 * 更時清掃（每次造訪章節僅一輪——已入帳的錢仍留在 Player 上）。Ch1 現有具體約 50
 * 的散布；Interlude 沒有（它的收益來源是攤位而非硬幣）。
 */
struct PickupPlacement {
    nccu::engine::math::Vec2 pos;   ///< 拾取點世界座標
    int             value;          ///< 撿取後入帳的金額
};

/**
 * @brief 取得指定章節狀態的零錢拾取點配置。
 * @param state 學期章節狀態。
 * @return 該狀態的拾取點向量（無拾取點的狀態回傳空向量）。
 */
inline const std::vector<PickupPlacement>& ChapterPickups(SemesterState state) {
    // Ch1 散布：10+10+20+5+5 = 50，落在開闊的指南路／中央校園地面，避開 5 個原型
    // NPC 生成點、4 把傘（y~1280、x 320-1560）與申請書 QuestFlagPickup（560,1725）。
    static const std::vector<PickupPlacement> kChapter1 = {
        {{ 760.0f, 1850.0f}, 10},
        {{1320.0f, 1850.0f}, 10},
        {{1080.0f, 1850.0f}, 20},
        {{1500.0f, 1430.0f},  5},
        {{ 600.0f, 1850.0f},  5},
    };
    // Ch2 散布：10+10+20 = 40（大於圖書館地下室自販機賣 EnergyDrink 的 35），沿
    // 撿筆記的路線分布，使身無分文抵達的玩家仍能湊出喚醒學霸的飲料錢——即 chapter2.md
    // 承諾的反死鎖底線。已遮罩驗證可走且避開筆記／NPC。
    static const std::vector<PickupPlacement> kChapter2 = {
        {{ 700.0f, 1750.0f}, 10},
        {{1050.0f, 1380.0f}, 10},
        {{ 950.0f,  700.0f}, 20},
    };
    static const std::vector<PickupPlacement> kChapter3;  // 尚未配置
    static const std::vector<PickupPlacement> kChapter4;  // 尚未配置
    static const std::vector<PickupPlacement> kNone;

    switch (state) {
        case SemesterState::Chapter1_AddDrop:   return kChapter1;
        case SemesterState::Chapter2_Midterms:  return kChapter2;
        case SemesterState::Chapter3_SportsDay: return kChapter3;
        case SemesterState::Chapter4_Finals:    return kChapter4;
        case SemesterState::Interlude_Market:
        case SemesterState::Ending_A:
        case SemesterState::Ending_B:
        case SemesterState::Ending_D:
        case SemesterState::Ending_C:
            return kNone;
    }
    return kNone;  // 不可達；使非 void 路徑完整
}

} // namespace nccu

#endif // CHAPTER_PICKUPS_H_
