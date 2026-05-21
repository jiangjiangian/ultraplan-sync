#include "quest/ChapterVendors.h"
#include "vendor/VendorLoader.h"
#include "gfx/Vec2.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace nccu {

namespace {

std::string& VendorContentDir() {
    static std::string dir = "docs/content";
    return dir;
}

// Spatial layout is code's job (the .md owns content/dialogue, not
// positions — same split as NpcSpawns vs chapter dialog). REQUIREMENT
// #7: the ten stalls are gathered into a TIGHT cluster at the dead
// CENTRE of 羅馬廣場 (the campus's circular plaza hub) — "全部移到羅馬
// 廣場正中間". If the parser yields fewer stalls than positions the
// extra spots are simply unused; if more, the surplus stalls fall back
// to the last position.
const std::vector<nccu::gfx::Vec2>& InterludeStallPositions() {
    // REQUIREMENT #7: a compact bullseye on the plaza CENTRE. The disc
    // centre is ≈(1088,960) (densest stone blob in worldmap_base.png;
    // the eight roads enter at rim ≈r200). collision_mask.png around the
    // centre is one continuous all-walkable box out to ≈r100 (verified
    // by sampling — the r=100 box is all-white; the only nearby wall is
    // the NE corner, outside ±100). Layout: 1 stall on the EXACT centre,
    // a tight inner ring r≈42 (3 stalls), an outer ring r≈78 (6 stalls)
    // — max radius 78 ≪ 100 (never near a road junction) and every pair
    // ≥ 35.9 px apart (> the 24 px Vendor collider, so no two stalls
    // overlap and the player can thread between them). Every point AND
    // its 24×24 collider footprint was verified pure-white (walkable) in
    // collision_mask.png and flood-fill-reachable from the player spawn;
    // test_spawn_reachability reads ChapterVendors().pos and re-checks
    // both invariants every build, plus a dedicated centred-cluster test
    // pins the centre / spread / no-overlap geometry.
    static const std::vector<nccu::gfx::Vec2> kPos = {
        {1088.0f,  960.0f},                                        // exact centre
        {1088.0f, 1002.0f}, {1051.6f,  939.0f}, {1124.4f,  939.0f}, // inner ring r42
        {1155.5f,  999.0f}, {1088.0f, 1038.0f}, {1020.5f,  999.0f}, // outer ring r78
        {1020.5f,  921.0f}, {1088.0f,  882.0f}, {1155.5f,  921.0f},
    };
    return kPos;
}

// Parsed-and-zipped Interlude placements, cached. std::optional so
// ReloadVendors() / SetVendorContentDir() can force a re-parse and the
// reference handed out by ChapterVendors() stays valid until then.
std::optional<std::vector<VendorPlacement>>& InterludeCache() {
    static std::optional<std::vector<VendorPlacement>> cache;
    return cache;
}

const std::vector<VendorPlacement>& BuildInterlude() {
    auto& cache = InterludeCache();
    if (cache) return *cache;

    std::vector<VendorPlacement> placements;
    const std::vector<VendorConfig> configs =
        nccu::vendor::LoadInterludeVendors(
            VendorContentDir() + "/interlude_market.md");
    const auto& pos = InterludeStallPositions();
    placements.reserve(configs.size());
    for (std::size_t i = 0; i < configs.size(); ++i) {
        const std::size_t p =
            pos.empty() ? 0
                        : (i < pos.size() ? i : pos.size() - 1);
        placements.push_back(VendorPlacement{
            configs[i],
            pos.empty() ? nccu::gfx::Vec2{0.0f, 0.0f} : pos[p]});
    }
    cache = std::move(placements);
    return *cache;
}

}  // namespace

// Ch2 圖書館地下室自動販賣機 (chapter2.md 學霸 (c-fail) / §五.3): the
// anti-softlock fallback so a player who did not buy an EnergyDrink in
// the market can still wake 學霸. One stall, EnergyDrink at the same 35
// the market charges (unlimited stock — a machine never sells out). A
// hand-written 3-field VendorConfig literal (the pinned aggregate
// contract); positioned on the playtested-walkable south band.
const std::vector<VendorPlacement>& Chapter2Vendors() {
    static const std::vector<VendorPlacement> kCh2 = {
        VendorPlacement{
            VendorConfig{"圖書館地下室自動販賣機",
                         "（投幣口閃著微光，機器嗡嗡作響）",
                         {VendorItem{"EnergyDrink", 35}}},
            nccu::gfx::Vec2{660.0f, 1850.0f}},
    };
    return kCh2;
}

// Ch4 集英樓便利商店 (chapter4.md L11/L295): the Ending C trigger.
// One stall selling the 超醜螢光綠雨傘; the buy SetFlag's
// Flag_BoughtUglyUmbrella (VendorItem::setsFlag, S5e-2b) and
// CheckEndingGates routes Ending C. Price is a fixed 100 — the
// chapter4.md「花光所有遊戲幣」is flavour, not a mechanic (a
// money-equal price would be fragile); unlimited stock. The ugly
// umbrella is deliberately NOT in the Interlude market (kept here so
// Ending C stays a Ch4 集英樓 act, per SCRIPT_HANDOFF / C.1).
const std::vector<VendorPlacement>& Chapter4Vendors() {
    static const std::vector<VendorPlacement> kCh4 = {
        VendorPlacement{
            VendorConfig{"集英樓便利商店",
                         "（貨架最下層，一把螢光綠到刺眼的傘）",
                         {VendorItem{"UglyUmbrella", 100, -1,
                                     "Flag_BoughtUglyUmbrella"}}},
            nccu::gfx::Vec2{1180.0f, 1725.0f}},
    };
    return kCh4;
}

const std::vector<VendorPlacement>& ChapterVendors(SemesterState state) {
    static const std::vector<VendorPlacement> kNone;
    if (state == SemesterState::Interlude_Market) return BuildInterlude();
    if (state == SemesterState::Chapter2_Midterms) return Chapter2Vendors();
    if (state == SemesterState::Chapter4_Finals)   return Chapter4Vendors();
    return kNone;  // Ch3 has no incidental Vendor (its quest is NPC trades)
}

void SetVendorContentDir(std::string dir) {
    VendorContentDir() = std::move(dir);
    InterludeCache().reset();   // dir change invalidates the parse
}

void ReloadVendors() {
    InterludeCache().reset();
}

}  // namespace nccu
