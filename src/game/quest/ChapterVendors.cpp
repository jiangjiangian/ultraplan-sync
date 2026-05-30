#include "game/quest/ChapterVendors.h"
#include "game/quest/Flags.h"
#include "game/vendor/VendorLoader.h"
#include "engine/math/Vec2.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

/**
 * @file ChapterVendors.cpp
 * @brief 各章節攤販的擺放來源：市集插曲由內容檔解析後配上座標，Ch2／Ch4 則為
 *        手寫的固定攤位；座標屬於 code 的職責，內容（文案、商品）屬於內容檔。
 */

namespace nccu {

namespace {

std::string& VendorContentDir() {
    static std::string dir = "docs/content";
    return dir;
}

// 空間擺放是 code 的職責（內容檔負責文案與對話，不負責座標——與 NpcSpawns
// 對章節對話的分工相同）。若解析出的攤位少於座標數，多出的位置就閒置不用；
// 若多於座標數，超出的攤位則一律退回最後一個位置。
const std::vector<nccu::engine::math::Vec2>& InterludeStallPositions() {
    // 在羅馬廣場（校園的圓形廣場樞紐）圓盤上排成整齊的兩排各五個：北排 y=900、
    // 南排 y=1020，中間留出約 120 px 的走道（y≈960）供玩家穿行、在兩側按 E。
    // 各欄 x∈{944,998,1052,1106,1160}（間距 54 px ⇒ 攤位間隙 30 px，大於 24 px
    // 的 Vendor 碰撞盒，故攤位彼此不重疊）。整個方陣以 x≈1052 為中心——比圓盤
    // 中心（1088）偏西約 36 px——因為廣場的東北、東南角有牆：可行走範圍在東側
    // 只到 x≈1160，若以圓盤中心擺放會太擠而難以穿行。每個點都經遮罩驗證為嚴格
    // 可行走（其 24×24 碰撞盒下方無實心像素）且可抵達（生成可達性測試每次建置
    // 都會重新檢查攤位座標，另有測試固定這套兩排幾何）。
    static const std::vector<nccu::engine::math::Vec2> kPos = {
        { 944.0f,  900.0f}, { 998.0f,  900.0f}, {1052.0f,  900.0f},
        {1106.0f,  900.0f}, {1160.0f,  900.0f},                       // 北排
        { 944.0f, 1020.0f}, { 998.0f, 1020.0f}, {1052.0f, 1020.0f},
        {1106.0f, 1020.0f}, {1160.0f, 1020.0f},                       // 南排
    };
    return kPos;
}

// 解析並與座標配對後的市集擺放結果，快取於此。用 std::optional 是為了讓
// ReloadVendors()／SetVendorContentDir() 能強制重新解析，且 ChapterVendors()
// 先前發出的參考在重新解析前都維持有效。
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
            pos.empty() ? nccu::engine::math::Vec2{0.0f, 0.0f} : pos[p]});
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
            nccu::engine::math::Vec2{980.0f, 560.0f}},
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
                                              kFlagBoughtUglyUmbrella}},
                         .spriteOverride = "resources/assets/Pixel Art "
                             "Vending Machines Pack/Machine 3/"
                             "Vending Machine 3.1.png"},
            nccu::engine::math::Vec2{1500.0f, 1450.0f}},
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
