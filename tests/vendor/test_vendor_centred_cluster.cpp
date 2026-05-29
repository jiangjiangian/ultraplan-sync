// Layout + #6 regression for the Interlude market stalls.
//
// Layout (player request, supersedes the old "正中間 tight cluster"
//     REQUIREMENT #7): the ten 羅馬廣場 stalls form TWO ROWS OF FIVE — a
//     north row and a south row — with a walkable middle aisle, all inside
//     the r≈200 plaza disc and non-overlapping (24 px colliders).
//     test_spawn_reachability separately proves every one is walkable +
//     flood-reachable on the shipped mask.
//
// #6: every stall must be a DIFFERENT person — the per-stall sprite key
//     (built from the stall's unique 攤主/name) and the curated
//     per-index fallback list must yield DISTINCT sprites for all ten
//     (the old code passed the literal "vendor" + one shop_auntie.png
//     for every stall → ten clones on a clean clone).
//
// Revert-verify:
//   * layout: collapse kPos back to one Y (or to the old centred bullseye)
//     — the "exactly two rows of five" / aisle CHECKs fail.
//   * #6: restore the single `PickNpcSprite("vendor", …, shop_auntie)`
//     for every stall — the "10 distinct fallback sprites" CHECK fails
//     (all ten identical).

#include "doctest/doctest.h"
#include "game/quest/ChapterVendors.h"
#include "game/vendor/VendorSprite.h"      // the EXACT production selector (#6)
#include "game/state/SemesterState.h"

#include <cmath>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

TEST_CASE("market stalls form two rows of five across the plaza") {
    nccu::SetVendorContentDir(TEST_CONTENT_DIR);
    nccu::ReloadVendors();
    const auto& m = nccu::ChapterVendors(
        nccu::SemesterState::Interlude_Market);
    REQUIRE(m.size() == 10);

    // Two rows (two distinct Y bands), five stalls each, with a walkable
    // middle aisle the player walks down to shop either side.
    std::map<float, int> rows;
    for (const auto& v : m) rows[v.pos.y]++;
    CHECK(rows.size() == 2);
    for (const auto& [y, n] : rows) CHECK(n == 5);
    const float y0 = rows.begin()->first;
    const float y1 = std::next(rows.begin())->first;
    CHECK(std::fabs(y1 - y0) >= 60.0f);          // aisle wide enough to walk

    // Every stall sits inside the walkable 羅馬廣場 disc (centre
    // ≈1088,960, the r≈200 stone circle), clear of the rim road junctions.
    constexpr float kCx = 1088.0f, kCy = 960.0f;
    for (const auto& v : m)
        CHECK(std::hypot(v.pos.x - kCx, v.pos.y - kCy) <= 160.0f);

    // No two stalls overlap: the Vendor collider is 24 px, so every
    // pair must be > 24 px apart (30 px comfort floor; the two-row pitch
    // is 66 px columns / 120 px rows, true minimum 66 px).
    for (std::size_t i = 0; i < m.size(); ++i)
        for (std::size_t j = i + 1; j < m.size(); ++j) {
            const float d = std::hypot(m[i].pos.x - m[j].pos.x,
                                       m[i].pos.y - m[j].pos.y);
            CHECK(d > 30.0f);
        }
}

TEST_CASE("REQ#6: every market stall maps to a DISTINCT sprite") {
    nccu::SetVendorContentDir(TEST_CONTENT_DIR);
    nccu::ReloadVendors();
    const auto& m = nccu::ChapterVendors(
        nccu::SemesterState::Interlude_Market);
    REQUIRE(m.size() == 10);

    // (a) The per-stall sprite KEY (the EXACT production helper) is
    //     unique per stall — built from the stall's own 攤主 (or name),
    //     which interlude_market.md authors distinctly. If keys
    //     collided, the PIPOYA path (pack present) would hash several
    //     stalls onto the same sprite (the old "vendor"-for-all bug).
    std::set<std::string> keys;
    for (const auto& v : m)
        keys.insert(nccu::VendorSpriteKey(v.config.stallKeeper,
                                          v.config.name));
    CHECK(keys.size() == m.size());          // all 10 keys distinct

    // (b) The sprite the spawn ACTUALLY assigns each stall (the exact
    //     World::SpawnChapterNpcs selector, VendorSpriteFor) is distinct
    //     for all ten — on a clean clone (no PIPOYA pack) this is the
    //     curated per-index fallback; the old single-sprite code made
    //     ten identical clones here, so reverting World.cpp's use of
    //     VendorSpriteFor (or collapsing the fallback list to one
    //     sprite) fails this CHECK.
    std::set<std::string> sprites;
    for (std::size_t i = 0; i < m.size(); ++i)
        sprites.insert(nccu::VendorSpriteFor(
            i, m[i].config.stallKeeper, m[i].config.name, m[i].pos));
    CHECK(sprites.size() == m.size());       // 10 distinct sprites assigned
}
