#include "doctest/doctest.h"
#include "game/quest/ChapterVendors.h"
#include "game/vendor/VendorSprite.h"      // 與正式產品完全相同的精靈選擇器
#include "game/state/SemesterState.h"

#include <cmath>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

/**
 * @file test_vendor_centred_cluster.cpp
 * @brief 驗證 Interlude 市集攤位的版面與精靈：十攤排成「兩排各五」並留出可行走的中央走道，
 *        全在廣場圓盤內且互不重疊；且每攤都對應到「不同」的精靈（避免十個分身）。
 */

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

// 市集十攤橫跨廣場排成兩排各五。
TEST_CASE("市集攤位橫跨廣場排成兩排各五攤") {
    nccu::SetVendorContentDir(TEST_CONTENT_DIR);
    nccu::ReloadVendors();
    const auto& m = nccu::ChapterVendors(
        nccu::SemesterState::Interlude_Market);
    REQUIRE(m.size() == 10);

    // 兩排（兩個不同的 Y 帶），各五攤，中間留出可行走的走道，玩家可沿走道往下到任一側採買。
    std::map<float, int> rows;
    for (const auto& v : m) rows[v.pos.y]++;
    CHECK(rows.size() == 2);
    for (const auto& [y, n] : rows) CHECK(n == 5);
    const float y0 = rows.begin()->first;
    const float y1 = std::next(rows.begin())->first;
    CHECK(std::fabs(y1 - y0) >= 60.0f);          // 走道夠寬可通行

    // 每攤都落在可行走的羅馬廣場圓盤內（中心約 1088,960，半徑約 200 的石圈），遠離邊緣路口。
    constexpr float kCx = 1088.0f, kCy = 960.0f;
    for (const auto& v : m)
        CHECK(std::hypot(v.pos.x - kCx, v.pos.y - kCy) <= 160.0f);

    // 任兩攤互不重疊：Vendor 碰撞箱為 24 px，故每一對距離須 > 24 px（取 30 px 為舒適下限；
    // 兩排的間距為欄距 66 px / 排距 120 px，真實最小值 66 px）。
    for (std::size_t i = 0; i < m.size(); ++i)
        for (std::size_t j = i + 1; j < m.size(); ++j) {
            const float d = std::hypot(m[i].pos.x - m[j].pos.x,
                                       m[i].pos.y - m[j].pos.y);
            CHECK(d > 30.0f);
        }
}

// 每個市集攤位都對應到「不同」的精靈。
TEST_CASE("每個市集攤位都對應到相異的精靈") {
    nccu::SetVendorContentDir(TEST_CONTENT_DIR);
    nccu::ReloadVendors();
    const auto& m = nccu::ChapterVendors(
        nccu::SemesterState::Interlude_Market);
    REQUIRE(m.size() == 10);

    // (a) 每攤的精靈 KEY（與正式產品完全相同的 helper）逐攤唯一——由攤位自己的攤主（或
    //     名稱）建出，內容作者撰寫時各不相同。若 key 相撞，有精靈包時會把多個攤位雜湊到
    //     同一個精靈。
    std::set<std::string> keys;
    for (const auto& v : m)
        keys.insert(nccu::VendorSpriteKey(v.config.stallKeeper,
                                          v.config.name));
    CHECK(keys.size() == m.size());          // 十個 key 皆相異

    // (b) 生成時實際指派給每攤的精靈（與 World::SpawnChapterNpcs 完全相同的選擇器
    //     VendorSpriteFor）對十攤皆相異——在乾淨環境（無精靈包）下這是精心安排的逐索引
    //     後備清單；舊的單一精靈程式會在此產生十個相同分身。
    std::set<std::string> sprites;
    for (std::size_t i = 0; i < m.size(); ++i)
        sprites.insert(nccu::VendorSpriteFor(
            i, m[i].config.stallKeeper, m[i].config.name, m[i].pos));
    CHECK(sprites.size() == m.size());       // 指派出十個相異精靈
}
