#ifndef CHAPTER_VENDORS_H_
#define CHAPTER_VENDORS_H_
#include "VendorConfig.h"
#include "SemesterState.h"
#include "gfx/Vec2.h"
#include <vector>

namespace nccu {

// Per-state Vendor roster, the price-table sibling of ChapterNpcSpawns.
// A Vendor is NOT an NpcSpawn (it needs a VendorConfig, not a sprite
// path + npcId), so it gets its own placement table. World iterates this
// alongside ChapterNpcSpawns inside RespawnChapterRoster: empty here =
// zero behaviour change, so S5b-2 only proves the spawn MECHANISM. The
// 10-stall Interlude lineup is transcribed from interlude_market.md §10
// (a list-for-review content gate) and lands in S5b-3; chapters get
// their incidental Vendors in S5c/d/e.
struct VendorPlacement {
    VendorConfig    config;
    nccu::gfx::Vec2 pos;
};

inline const std::vector<VendorPlacement>& ChapterVendors(SemesterState state) {
    static const std::vector<VendorPlacement> kInterlude;  // TODO(S5b-3): 10 stalls
    static const std::vector<VendorPlacement> kChapter2;   // TODO(S5c)
    static const std::vector<VendorPlacement> kChapter3;   // TODO(S5d)
    static const std::vector<VendorPlacement> kChapter4;   // TODO(S5e)
    static const std::vector<VendorPlacement> kNone;

    switch (state) {
        case SemesterState::Interlude_Market:   return kInterlude;
        case SemesterState::Chapter2_Midterms:  return kChapter2;
        case SemesterState::Chapter3_SportsDay: return kChapter3;
        case SemesterState::Chapter4_Finals:    return kChapter4;
        case SemesterState::Chapter1_AddDrop:
        case SemesterState::Ending_A:
        case SemesterState::Ending_B:
        case SemesterState::Ending_C:
            return kNone;
    }
    return kNone;  // unreachable; keeps non-void paths total
}

} // namespace nccu

#endif // CHAPTER_VENDORS_H_
