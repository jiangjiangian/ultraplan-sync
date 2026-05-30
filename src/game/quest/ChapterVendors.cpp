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

// Ch2 圖書館地下室自動販賣機：防卡關的後備手段，讓沒在市集買到 EnergyDrink
// 的玩家仍能喚醒學霸。單一攤位，EnergyDrink 售價與市集相同（35），庫存無限——
// 販賣機不會售罄。以手寫的三欄位 VendorConfig 字面常數建構。位置擺在中正圖書館
// 正面（圖書館矩形南緣、管理員櫃台旁），使「圖書館地下室自販機」這個名稱與其
// 所在地相符——舊的南側帶狀位置距它命名所指的圖書館約 1.3k px（曾被地圖檢查的
// Ch2 攤販對中正圖書館 的預期檢查抓到）。
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

// Ch4 集英樓便利商店：Ending C 的觸發點。單一攤位販售超醜螢光綠雨傘；購買時
// 透過 VendorItem::setsFlag 設定 kFlagBoughtUglyUmbrella，CheckEndingGates 據此
// 導向 Ending C。價格固定為 100——劇本中「花光所有遊戲幣」是氛圍敘述而非機制
// （以等同身上金錢的價格會很脆弱）；庫存無限。這把醜傘刻意「不」放進市集插曲，
// 留在此處，使 Ending C 維持為 Ch4 集英樓 的橋段。渲染為販賣機，並移到集英樓
// 矩形 (1524,1353)-(1748,1545) 的西側面，使其終於座落在命名所指的集英樓——舊的
// 位置偏西約 340 px，離該建物很遠。採用 Machine 3 的美術以與 Ch2 圖書館販賣機
//（Machine 1）區隔。
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
    return kNone;  // Ch3 沒有附帶的 Vendor（其任務是 NPC 之間的物物交換）
}

void SetVendorContentDir(std::string dir) {
    VendorContentDir() = std::move(dir);
    InterludeCache().reset();   // 目錄改變即令先前的解析結果失效
}

void ReloadVendors() {
    InterludeCache().reset();
}

}  // namespace nccu
