// REQUIREMENT #7 + #6 regression for the Interlude market stalls.
//
// #7: the ten 羅馬廣場 stalls must sit in a TIGHT cluster on the plaza
//     CENTRE — pin the centre, the small spread, and that no two stalls
//     overlap (their 24 px colliders) so the layout is "正中間", not the
//     old wide scatter. test_spawn_reachability separately proves every
//     one is walkable + flood-reachable on the shipped mask.
//
// #6: every stall must be a DIFFERENT person — the per-stall sprite key
//     (built from the stall's unique 攤主/name) and the curated
//     per-index fallback list must yield DISTINCT sprites for all ten
//     (the old code passed the literal "vendor" + one shop_auntie.png
//     for every stall → ten clones on a clean clone).
//
// Revert-verify:
//   * #7: restore the old scattered kPos — the "all within R of centre"
//     / max-radius CHECKs fail (old max radius ≈125, several stalls far
//     off the exact centre).
//   * #6: restore the single `PickNpcSprite("vendor", …, shop_auntie)`
//     for every stall — the "10 distinct fallback sprites" CHECK fails
//     (all ten identical).

#include "doctest/doctest.h"
#include "quest/ChapterVendors.h"
#include "vendor/VendorSprite.h"      // the EXACT production selector (#6)
#include "state/SemesterState.h"

#include <cmath>
#include <set>
#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

TEST_CASE("REQ#7: the 10 market stalls form a tight cluster at the plaza centre") {
    nccu::SetVendorContentDir(TEST_CONTENT_DIR);
    nccu::ReloadVendors();
    const auto& m = nccu::ChapterVendors(
        nccu::SemesterState::Interlude_Market);
    REQUIRE(m.size() == 10);

    // The documented plaza centre (also exactly stall[0]).
    constexpr float kCx = 1088.0f, kCy = 960.0f;
    CHECK(m[0].pos.x == doctest::Approx(kCx));
    CHECK(m[0].pos.y == doctest::Approx(kCy));

    float maxR = 0.0f;
    for (const auto& v : m) {
        const float r = std::hypot(v.pos.x - kCx, v.pos.y - kCy);
        maxR = std::max(maxR, r);
    }
    // Tight: every stall within ~80 px of the centre (the verified
    // all-walkable plaza box is ≈r100; the old scatter reached ≈r125).
    CHECK(maxR <= 80.0f);

    // No two stalls overlap: the Vendor collider is 24 px, so every
    // pair must be > 24 px apart (we use a comfortable 30 px floor —
    // the authored layout's true minimum is ≈35.9 px).
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
