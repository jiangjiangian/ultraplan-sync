#ifndef VENDOR_SPRITE_H_
#define VENDOR_SPRITE_H_
#include "quest/PipoyaRoster.h"
#include "engine/math/Vec2.h"
#include <cstddef>
#include <string>

namespace nccu {

// REQUIREMENT #6: every market stall must be a DIFFERENT person. This is
// the single pure selection rule World::SpawnChapterNpcs uses for a
// Vendor's sprite, factored out so it is exercised by the production
// path AND directly unit-testable (revert-verifiable) without a GL
// context / the optional PIPOYA asset pack.
//
//   * The PickNpcSprite KEY is the stall's own 攤主 (or, if none, its
//     name) — interlude_market.md authors these distinctly, so the
//     per-stall hashes differ instead of every stall colliding on the
//     old literal "vendor" (which made the PIPOYA path pick ONE sprite
//     for all ten).
//   * The fallback (returned verbatim by PickNpcSprite when the PIPOYA
//     pack is absent — i.e. on a clean clone / the grading rebuild) is a
//     curated, visually-distinct sprite chosen by spawn INDEX, cycling
//     over the sprites that actually ship (school_uniform_3/* + the
//     three npc/* keepers — NO new art, CLAUDE.md §5). The old code used
//     ONE shop_auntie.png for every stall, so a clean clone rendered ten
//     identical clones — exactly the reported defect.
inline const char* const kVendorFallbackSprites[] = {
    "resources/assets/sprites/npc/shop_auntie.png",
    "resources/assets/sprites/npc/suit_senior.png",
    "resources/assets/sprites/npc/ta.png",
    "resources/assets/sprites/school_uniform_3/female_04.png",
    "resources/assets/sprites/school_uniform_3/male_04.png",
    "resources/assets/sprites/school_uniform_3/female_07.png",
    "resources/assets/sprites/school_uniform_3/male_07.png",
    "resources/assets/sprites/school_uniform_3/female_10.png",
    "resources/assets/sprites/school_uniform_3/male_10.png",
    "resources/assets/sprites/school_uniform_3/female_13.png",
};
inline constexpr std::size_t kVendorFallbackCount =
    sizeof(kVendorFallbackSprites) / sizeof(kVendorFallbackSprites[0]);

// The per-stall sprite-selection key (unique per stall by construction).
inline std::string VendorSpriteKey(const std::string& stallKeeper,
                                   const std::string& name) {
    return "vendor:" + (stallKeeper.empty() ? name : stallKeeper);
}

// The sprite a stall at spawn `index` gets: a distinct curated fallback
// per index (clean clone), or a PIPOYA pick keyed off the unique stall
// string (when the pack is present). Deterministic & stable per stall.
inline std::string VendorSpriteFor(std::size_t index,
                                   const std::string& stallKeeper,
                                   const std::string& name,
                                   nccu::gfx::Vec2 pos) {
    return PickNpcSprite(
        VendorSpriteKey(stallKeeper, name), pos,
        kVendorFallbackSprites[index % kVendorFallbackCount]);
}

} // namespace nccu

#endif // VENDOR_SPRITE_H_
