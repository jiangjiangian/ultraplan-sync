#include "doctest/doctest.h"
#include "game/vendor/VendorLoader.h"
#include "game/quest/ChapterVendors.h"
#include "game/state/SemesterState.h"

#include <string>
#include <vector>

/**
 * @file test_vendor_loader.cpp
 * @brief 驗證 LoadInterludeVendors 解析市集攤位內容檔的契約（內容檔為唯一真實來源）：
 *        解析全部十攤、募款箱帶業力與有限庫存、變體成功區塊只取第一個，以及 ChapterVendors
 *        會以解析結果定位並快取。對真實內容檔測試，使破壞格式的作者編輯能在此被攔下。
 */

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

using nccu::vendor::LoadInterludeVendors;

namespace {

std::string ContentPath() {
    return std::string(TEST_CONTENT_DIR) + "/interlude_market.md";
}

const VendorConfig* Find(const std::vector<VendorConfig>& v,
                         const std::string& name) {
    for (const auto& c : v) if (c.name == name) return &c;
    return nullptr;
}

}  // namespace

// LoadInterludeVendors 從真實內容解析全部十攤。
TEST_CASE("LoadInterludeVendors: parses all 10 stalls from real content") {
    const auto stalls = LoadInterludeVendors(ContentPath());
    REQUIRE(stalls.size() == 10);

    // --- 攤位 1：熱騰騰雞排攤——Tier-1 的真實消耗品 -------------------
    const auto& s1 = stalls[0];
    CHECK(s1.name        == "熱騰騰雞排攤");
    CHECK(s1.stallKeeper == "炸物阿姨");
    CHECK(s1.mechanic    == "buy");
    CHECK(s1.tier        == 1);
    REQUIRE(s1.stock.size() == 1);
    CHECK(s1.stock[0].itemId    == "HotPack");
    CHECK(s1.stock[0].price     == 25);
    CHECK(s1.stock[0].stockLeft == -1);          // 無 stock 標記 -> 無限
    REQUIRE(s1.greetingLines.size() == 3);
    CHECK(s1.greetingLines[0] == "同學淋這麼濕，吃塊雞排暖一下。");
    CHECK(s1.greeting         == s1.greetingLines[0]);  // 單行時的後備
    CHECK(s1.onPurchase.size() == 2);
    CHECK(s1.onLeave.size()    == 1);
}

// 募款箱帶有業力與有限庫存。
TEST_CASE("LoadInterludeVendors: 募款箱 carries karma + finite stock") {
    const auto stalls = LoadInterludeVendors(ContentPath());
    const VendorConfig* d = Find(stalls, "學生會募款箱");
    REQUIRE(d != nullptr);
    CHECK(d->mechanic        == "donate");
    CHECK(d->karmaOnInteract == 1);              // 業力：+1
    REQUIRE(d->stock.size() == 1);
    CHECK(d->stock[0].itemId    == "Donation");
    CHECK(d->stock[0].price     == 10);
    CHECK(d->stock[0].stockLeft == 5);           // 套用 stock：5
}

// 多個成功區塊時——第一個生效、且不給物品。
TEST_CASE("LoadInterludeVendors: variant success blocks — first wins, no item") {
    const auto stalls = LoadInterludeVendors(ContentPath());
    const VendorConfig* sell = Find(stalls, "畢業生二手書攤");
    REQUIRE(sell != nullptr);
    CHECK(sell->mechanic == "sell");
    CHECK(sell->stock.empty());                  // 無「商品」行
    // onPurchase（陷阱傘殘骸）是第一個成功區塊；（其他物資）變體會被解析但丟棄（只保留第一個）。
    REQUIRE(sell->onPurchase.size() == 2);
    CHECK(sell->onPurchase[0] == "這把傘骨架怎麼這樣……算了，我拆材料用。");
}

// ChapterVendors：Interlude 由解析器供給、已定位且有快取。
TEST_CASE("ChapterVendors: Interlude is parser-backed + positioned + cached") {
    nccu::SetVendorContentDir(TEST_CONTENT_DIR);
    nccu::ReloadVendors();

    const auto& v = nccu::ChapterVendors(
        nccu::SemesterState::Chapter1_AddDrop);
    CHECK(v.empty());                            // 一般章節：尚無攤位

    const auto& m = nccu::ChapterVendors(
        nccu::SemesterState::Interlude_Market);
    REQUIRE(m.size() == 10);
    CHECK(m[0].config.name == "熱騰騰雞排攤");
    // 攤位群聚在廣場中心（約 1088,960），故兩攤可能共用同一 X（或 Y）卻仍是不同的點。此處
    // 斷言真實意圖：十個程式端位置兩兩相異、皆遠在南側出口帶以北、且群聚在廣場中心（最大
    // 半徑很小）。可行走／可達的證明由 test_spawn_reachability 負責；中心／散布幾何由
    // test_vendor_centred_cluster 負責。
    for (std::size_t i = 0; i < m.size(); ++i) {
        CHECK(m[i].pos.y < 1900.0f);
        for (std::size_t j = i + 1; j < m.size(); ++j) {
            // doctest 不允許在 CHECK 內用 `&&`——先化簡為單一 bool。
            const bool samePoint = (m[i].pos.x == m[j].pos.x) &&
                                   (m[i].pos.y == m[j].pos.y);
            CHECK_FALSE(samePoint);                  // 兩兩相異
        }
    }

    // 已快取：第二次呼叫回傳同一份底層儲存。
    const auto& m2 = nccu::ChapterVendors(
        nccu::SemesterState::Interlude_Market);
    CHECK(&m == &m2);
}
