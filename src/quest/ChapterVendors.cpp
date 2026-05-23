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
    // Two tidy rows of five across the 羅馬廣場 disc (player request,
    // superseding the old "正中間 tight cluster" REQUIREMENT #7): a north
    // row at y=900 and a south row at y=1020, leaving a ~120-px middle
    // aisle (y≈960) the player walks down to press E on either side.
    // Columns at x∈{944,998,1052,1106,1160} (54-px pitch ⇒ 30-px gaps >
    // the 24-px Vendor collider, so stalls never overlap). The grid is
    // centred on x≈1052 — ~36 px WEST of the disc centre (1088) — because
    // the plaza's NE and SE corners are walled (map_registry.py walkability
    // map): the walkable box only reaches x≈1160 on the east, so a disc-
    // centred grid would be too cramped to thread. Every point was mask-
    // verified STRICTLY walkable (no solid pixel under its 24×24 box) and
    // reachable (test_spawn_reachability re-checks ChapterVendors().pos
    // every build; test_vendor_centred_cluster pins the two-row geometry).
    static const std::vector<nccu::gfx::Vec2> kPos = {
        { 944.0f,  900.0f}, { 998.0f,  900.0f}, {1052.0f,  900.0f},
        {1106.0f,  900.0f}, {1160.0f,  900.0f},                       // north row
        { 944.0f, 1020.0f}, { 998.0f, 1020.0f}, {1052.0f, 1020.0f},
        {1106.0f, 1020.0f}, {1160.0f, 1020.0f},                       // south row
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
// contract). Positioned at the 中正圖書館 front (just south of the
// library rect, beside the 管理員 desk) so the "圖書館地下室自販機" name
// matches where it stands — the old (660,1850) south-band spot sat
// ~1.3k px from the library it is named after (caught by
// map_registry.py's Ch2 vendor↔中正圖書館 expectation check).
const std::vector<VendorPlacement>& Chapter2Vendors() {
    static const std::vector<VendorPlacement> kCh2 = {
        VendorPlacement{
            VendorConfig{.name = "圖書館地下室自動販賣機",
                         .greeting = "（投幣口閃著微光，機器嗡嗡作響）",
                         .stock = {VendorItem{"EnergyDrink", 35}},
                         .spriteOverride = "resources/assets/Pixel Art "
                             "Vending Machines Pack/Machine 1/"
                             "Vending Machine 1.1.png"},
            nccu::gfx::Vec2{980.0f, 560.0f}},
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
// Rendered as a vending machine (player request) and moved to the west
// face of 集英樓 (1524,1353)-(1748,1545) so it finally stands AT the
// 集英樓 it is named after — the old (1180,1725) spot sat ~340 px west,
// nowhere near the building (position audit). Machine 3 art distinguishes
// it from the Ch2 圖書館 machine (Machine 1).
const std::vector<VendorPlacement>& Chapter4Vendors() {
    static const std::vector<VendorPlacement> kCh4 = {
        VendorPlacement{
            VendorConfig{.name = "集英樓便利商店",
                         .greeting = "（貨架最下層，一把螢光綠到刺眼的傘）",
                         .stock = {VendorItem{"UglyUmbrella", 100, -1,
                                              "Flag_BoughtUglyUmbrella"}},
                         .spriteOverride = "resources/assets/Pixel Art "
                             "Vending Machines Pack/Machine 3/"
                             "Vending Machine 3.1.png"},
            nccu::gfx::Vec2{1500.0f, 1450.0f}},
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
