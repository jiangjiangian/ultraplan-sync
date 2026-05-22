#ifndef CHAPTER_PICKUPS_H_
#define CHAPTER_PICKUPS_H_
#include "state/SemesterState.h"
#include "gfx/Vec2.h"
#include <vector>

namespace nccu {

// Per-state CashPickup placements — the探索 (exploration) earner of the
// S5b-4 loop economy, sibling of ChapterNpcSpawns / ChapterVendors. The
// player who sweeps the map for loose cash funds the next market; the
//德行 earner is karma-gated NPC tips (per-chapter, S5c/d/e) and the
// 策略 earner is the market itself (募款箱 / 二手書, S5b-3).
//
// World spawns these in SpawnChapterNpcs and tracks them in the chapter
// roster, so uncollected coins are swept on the next state change (one
// shot per chapter visit — money already banked persists on the Player).
// Ch1 carries a concrete ~50-money spread now; Ch2-4 are filled by
// S5c/d/e; the Interlude has none (its earners are stalls, not coins).
struct PickupPlacement {
    nccu::gfx::Vec2 pos;
    int             value;
};

inline const std::vector<PickupPlacement>& ChapterPickups(SemesterState state) {
    // Ch1 spread: 10+10+20+5+5 = 50, on open Zhinan / central-campus
    // ground clear of the 5 archetype spawns, the 4 umbrellas (y~1280,
    // x 320-1560) and the 申請書 QuestFlagPickup (560,1725).
    static const std::vector<PickupPlacement> kChapter1 = {
        {{ 760.0f, 1850.0f}, 10},
        {{1320.0f, 1850.0f}, 10},
        {{1080.0f, 1850.0f}, 20},
        {{1500.0f, 1430.0f},  5},
        {{ 600.0f, 1850.0f},  5},
    };
    // Ch2 spread: 10+10+20 = 40 (> the 35 the 圖書館地下室自販機 charges
    // for the EnergyDrink), strung along the note tour so a player who
    // arrives broke can still scrape together the wake-學霸 drink — the
    // anti-softlock floor chapter2.md §五.3 promises. Mask-verified
    // walkable and clear of the notes / NPCs (map_registry.py).
    static const std::vector<PickupPlacement> kChapter2 = {
        {{ 700.0f, 1750.0f}, 10},
        {{1050.0f, 1380.0f}, 10},
        {{ 950.0f,  700.0f}, 20},
    };
    static const std::vector<PickupPlacement> kChapter3;  // TODO(S5d)
    static const std::vector<PickupPlacement> kChapter4;  // TODO(S5e)
    static const std::vector<PickupPlacement> kNone;

    switch (state) {
        case SemesterState::Chapter1_AddDrop:   return kChapter1;
        case SemesterState::Chapter2_Midterms:  return kChapter2;
        case SemesterState::Chapter3_SportsDay: return kChapter3;
        case SemesterState::Chapter4_Finals:    return kChapter4;
        case SemesterState::Interlude_Market:
        case SemesterState::Ending_A:
        case SemesterState::Ending_B:
        case SemesterState::Ending_C:
            return kNone;
    }
    return kNone;  // unreachable; keeps non-void paths total
}

} // namespace nccu

#endif // CHAPTER_PICKUPS_H_
