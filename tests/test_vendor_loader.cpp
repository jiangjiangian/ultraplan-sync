#include "doctest/doctest.h"
#include "VendorLoader.h"
#include "ChapterVendors.h"
#include "SemesterState.h"

#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

// S5b-3: LoadInterludeVendors is the runtime parser for the normalized
// interlude_market.md §10 (the .md is the single source of truth, like
// chapter dialog). This pins the parse contract against the REAL content
// file so an authoring edit that breaks the format is caught.

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

TEST_CASE("LoadInterludeVendors: parses all 10 stalls from real content") {
    const auto stalls = LoadInterludeVendors(ContentPath());
    REQUIRE(stalls.size() == 10);

    // --- Stall 1: 熱騰騰雞排攤 — Tier-1 real consumable -------------------
    const auto& s1 = stalls[0];
    CHECK(s1.name        == "熱騰騰雞排攤");
    CHECK(s1.stallKeeper == "炸物阿姨");
    CHECK(s1.mechanic    == "buy");
    CHECK(s1.tier        == 1);
    REQUIRE(s1.stock.size() == 1);
    CHECK(s1.stock[0].itemId    == "HotPack");
    CHECK(s1.stock[0].price     == 25);
    CHECK(s1.stock[0].stockLeft == -1);          // no `> stock：` -> unlimited
    REQUIRE(s1.greetingLines.size() == 3);
    CHECK(s1.greetingLines[0] == "同學淋這麼濕，吃塊雞排暖一下。");
    CHECK(s1.greeting         == s1.greetingLines[0]);  // single-line fallback
    CHECK(s1.onPurchase.size() == 2);
    CHECK(s1.onLeave.size()    == 1);
}

TEST_CASE("LoadInterludeVendors: 募款箱 carries karma + finite stock") {
    const auto stalls = LoadInterludeVendors(ContentPath());
    const VendorConfig* d = Find(stalls, "學生會募款箱");
    REQUIRE(d != nullptr);
    CHECK(d->mechanic        == "donate");
    CHECK(d->karmaOnInteract == 1);              // `> karma：+1`
    REQUIRE(d->stock.size() == 1);
    CHECK(d->stock[0].itemId    == "Donation");
    CHECK(d->stock[0].price     == 10);
    CHECK(d->stock[0].stockLeft == 5);           // `> stock：5` applied
}

TEST_CASE("LoadInterludeVendors: variant success blocks — first wins, no item") {
    const auto stalls = LoadInterludeVendors(ContentPath());
    const VendorConfig* sell = Find(stalls, "畢業生二手書攤");
    REQUIRE(sell != nullptr);
    CHECK(sell->mechanic == "sell");
    CHECK(sell->stock.empty());                  // no `> 商品：` line
    // onPurchase（陷阱傘殘骸） is the first success block; the（其他物資）
    // variant is parsed-but-dropped (Phase 2 keeps the first).
    REQUIRE(sell->onPurchase.size() == 2);
    CHECK(sell->onPurchase[0] == "這把傘骨架怎麼這樣……算了，我拆材料用。");
}

TEST_CASE("ChapterVendors: Interlude is parser-backed + positioned + cached") {
    nccu::SetVendorContentDir(TEST_CONTENT_DIR);
    nccu::ReloadVendors();

    const auto& v = nccu::ChapterVendors(
        nccu::SemesterState::Chapter1_AddDrop);
    CHECK(v.empty());                            // chapters: none yet

    const auto& m = nccu::ChapterVendors(
        nccu::SemesterState::Interlude_Market);
    REQUIRE(m.size() == 10);
    CHECK(m[0].config.name == "熱騰騰雞排攤");
    // REQUIREMENT #7 reconciliation: the stalls are now a tight bullseye
    // on the plaza CENTRE (≈1088,960), so two stalls may share an X (or
    // Y) while still being DISTINCT points — the old `m[0].x != m[1].x`
    // assumed the scattered layout. Assert the real intent instead: all
    // ten code-side positions are pairwise distinct, well north of the
    // south exit band, and clustered at the plaza centre (max radius
    // small). test_spawn_reachability owns the walkable/reachable proof;
    // test_vendor_centred_cluster owns the centre/spread geometry.
    for (std::size_t i = 0; i < m.size(); ++i) {
        CHECK(m[i].pos.y < 1900.0f);
        for (std::size_t j = i + 1; j < m.size(); ++j) {
            // doctest forbids `&&` inside CHECK — reduce to one bool.
            const bool samePoint = (m[i].pos.x == m[j].pos.x) &&
                                   (m[i].pos.y == m[j].pos.y);
            CHECK_FALSE(samePoint);                  // pairwise distinct
        }
    }

    // Cached: a second call returns the same backing storage.
    const auto& m2 = nccu::ChapterVendors(
        nccu::SemesterState::Interlude_Market);
    CHECK(&m == &m2);
}
