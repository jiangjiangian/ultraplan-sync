#ifndef GFX_TEXTURE_PRELOAD_H_
#define GFX_TEXTURE_PRELOAD_H_
#include "engine/render/Texture.h"
#include "game/gfx/Decorations.h"          // nccu::game::gfx::kDecorations — ambient strips
#include "game/entities/Personas.h"   // kPersonas — player + preview sheets
#include "game/world/Buildings.h"          // kAll — building art
#include "game/world/Obstacles.h"          // kBuildingCollisionSkip
#include "game/vendor/VendorSprite.h"      // kVendorFallbackSprites
#include "game/quest/PipoyaRoster.h"       // PipoyaRoster() — varied NPC art
#include <algorithm>
#include <array>
#include <string>
#include <string_view>

// UI-C-2: warm the process texture cache (gfx/Texture.h) with the textures
// the World/View will request, BEFORE the player enters gameplay — so the
// World ctor (entity LoadSprite) and the View ctor (worldmap + building +
// decoration strips) hit the warm cache instead of reading every file from
// disk + uploading to the GPU right as the run starts (the one-frame stutter
// this fixes). Also benefits Restart: re-entering builds a fresh World/View
// but the cache is already warm, so no second stutter.
//
// This is a PURE gfx concern: it only reads the static ART tables (building
// rects, persona sheets, decoration strips, the sprite roster) to know which
// FILES exist — never any gameplay/quest STATE — so it stays MVC-clean. The
// paths mirror exactly what View.cpp / World.cpp / CharacterSelect.cpp pass
// to Texture::Load, sourced from the same canonical tables so the list can't
// silently drift out of sync.
//
// Every warm is a no-op-cheap miss when the file is absent (a fresh clone /
// the headless harness has an empty resources/), so calling this on the
// headless path is essentially free — but the loading SCREEN that shows
// while it runs is gated to the human path only by the caller (main.cpp),
// since the harness skips the title/select screens for a byte-identical
// state.jsonl.
namespace nccu::game::world {

// The single worldmap base texture (largest single upload). Mirrors the
// path View::View passes to Texture::Load (ui/View.cpp).
inline constexpr std::string_view kWorldmapBasePath =
    "resources/assets/maps/worldmap_base.png";

// The curated story-NPC / vendor sprite sheets that are hardcoded in the
// quest spawn tables (ChapterSpawns.h) and the persona/vendor rosters but
// are NOT in kPersonas / kVendorFallbackSprites — kept here so the preload
// set is complete on a clean clone (the PIPOYA pack, when present, adds the
// rest via PipoyaRoster()). No new files: these all already ship/are
// referenced; a missing one is a cheap cache miss.
inline constexpr std::array<std::string_view, 16> kCuratedSprites{{
    "resources/assets/sprites/npc/shop_auntie.png",
    "resources/assets/sprites/npc/suit_senior.png",
    "resources/assets/sprites/npc/ta.png",
    "resources/assets/sprites/school_uniform_3/male_01.png",
    "resources/assets/sprites/school_uniform_3/male_02.png",
    "resources/assets/sprites/school_uniform_3/male_03.png",
    "resources/assets/sprites/school_uniform_3/male_04.png",
    "resources/assets/sprites/school_uniform_3/male_05.png",
    "resources/assets/sprites/school_uniform_3/female_01.png",
    "resources/assets/sprites/school_uniform_3/female_02.png",
    "resources/assets/sprites/school_uniform_3/female_03.png",
    "resources/assets/sprites/school_uniform_3/female_04.png",
    "resources/assets/sprites/school_uniform_3/female_05.png",
    "resources/assets/sprites/school_uniform_3/female_06.png",
    "resources/assets/sprites/school_uniform_3/female_07.png",
    "resources/assets/sprites/school_uniform_3/female_10.png",
}};

// Build the building-art path for a Building exactly as View::View does
// (ui/View.cpp): "resources/assets/buildings_3d_trimmed/<name>.png".
inline std::string BuildingTexturePath(std::string_view name) {
    std::string p = "resources/assets/buildings_3d_trimmed/";
    p += std::string(name);
    p += ".png";
    return p;
}

// Warm the cache with every texture the World/View will request. Idempotent
// (each path uploads at most once) and cheap on a clean clone (misses no-op).
// Safe to call once at startup; a later Restart simply re-hits the warm
// cache. MUST be called after InitWindow (GPU access) — the caller guards
// this with the live window, exactly like EnsureFont.
inline void PreloadGameTextures() {
    // 1. The world base map (one big texture).
    nccu::engine::render::PreloadTexture(std::string(kWorldmapBasePath));

    // 2. Building art — mirror View::View's skip set + path build.
    const auto& skip = obstacles::kBuildingCollisionSkip;
    for (const auto& b : buildings::kAll) {
        if (std::find(skip.begin(), skip.end(), b.name) != skip.end())
            continue;                       // open-ground baked into the base
        nccu::engine::render::PreloadTexture(BuildingTexturePath(b.name));
    }

    // 3. Ambient decoration strips (cosmetic) — same defs the View loads.
    for (const auto& d : nccu::game::gfx::kDecorations)
        nccu::engine::render::PreloadTexture(std::string(d.stripPath));

    // 4. The five persona sheets (player sprite + character-select previews).
    for (const auto& p : kPersonas)
        nccu::engine::render::PreloadTexture(std::string(p.spritePath));

    // 5. The vendor fallback roster (clean-clone stall sprites).
    for (std::size_t i = 0; i < kVendorFallbackCount; ++i)
        nccu::engine::render::PreloadTexture(std::string(kVendorFallbackSprites[i]));

    // 6. The curated story-NPC / extra uniform sheets.
    for (const auto& s : kCuratedSprites)
        nccu::engine::render::PreloadTexture(std::string(s));

    // 7. The varied NPC roster (PIPOYA pack) — directory-enumerated, so it
    // is empty (and this loop a no-op) on a clean clone, and warms the whole
    // crowd when the optional pack is present.
    for (const auto& s : PipoyaRoster())
        nccu::engine::render::PreloadTexture(s);
}

} // namespace nccu::game::world

#endif // GFX_TEXTURE_PRELOAD_H_
