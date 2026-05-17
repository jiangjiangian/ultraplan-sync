#include "ChapterVendors.h"
#include "VendorLoader.h"
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
// positions — same split as NpcSpawns vs chapter dialog). The ten stalls
// ring 羅馬廣場 — the campus's circular plaza hub — per the playtest
// request to gather the market inside the plaza circle and off the
// radiating road junctions ("路口"). If the parser yields fewer stalls
// than positions the extra spots are simply unused; if more, the surplus
// stalls fall back to the last position.
const std::vector<nccu::gfx::Vec2>& InterludeStallPositions() {
    // 2026-05-17: scattered across 羅馬廣場's interior. The plaza disc was
    // located from worldmap_base.png (densest filled stone blob): centre
    // ≈(1088,960), rim ≈r200 where the eight roads enter. The earlier
    // single-radius r90 ring looked like a fence encircling the plaza;
    // the playtest asked for the stalls SPREAD across the middle, not
    // ringed. So these ten sit at mixed radii — one at the centre, a
    // tight inner cluster (~r70) and a looser middle band (~r115-125),
    // angles jittered — staying well inside the rim (max r≈125 ≪ 200) so
    // none blocks a road junction. Every point was verified pure-white
    // (walkable) in collision_mask.png and is flood-fill-reachable from
    // the player spawn; test_spawn_reachability reads ChapterVendors()
    // .pos directly and re-checks both invariants on every build.
    static const std::vector<nccu::gfx::Vec2> kPos = {
        {1088.0f,  960.0f}, {1155.0f,  982.0f}, {1036.0f, 1007.0f},
        {1073.0f,  892.0f}, {1207.0f,  999.0f}, {1114.0f, 1082.0f},
        { 995.0f, 1044.0f}, { 969.0f,  921.0f}, {1065.0f,  849.0f},
        {1181.0f,  876.0f},
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
